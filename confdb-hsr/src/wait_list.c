#include "wait_list.h"
#include <string.h>
#include "apply_conf.h"
#include "log.h"

static int get_link(struct nl_sock *sk,
		    struct rtnl_link **r_link,
		    const char *link_name)
{
	struct nl_cache *link_cache;
	struct rtnl_link *link;
	int ret;

	ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	link = rtnl_link_get_by_name(link_cache, link_name);
	if (!link) {
		ret = -1;
		goto _error;
	}

	*r_link = link;
	ret = 0;
_error:
	nl_cache_put(link_cache);

	return ret;
}

int change_interface_data(struct hsr_module *app,
			  struct hsr_interface *intrf,
			  const char *interface_name,
			  const char *slave1_name,
			  const char *slave2_name)
{
	int ret;

	intrf->name = strdup(interface_name);
	intrf->slaves[0].name = strdup(slave1_name);
	intrf->slaves[1].name = strdup(slave2_name);
	printf("46_change_interface_data\n");
	rtnl_link_put(intrf->slaves[0].link);
	rtnl_link_put(intrf->slaves[1].link);
printf("49_change_interface_data\n");
	ret = get_link(app->route_manager->sk, &intrf->slaves[0].link, slave1_name);
	if (ret == -1) {
		DLOG_ERR("Slave1 is not exists");
		return ret;
	}
printf("55_change_interface_data\n");
	ret = get_link(app->route_manager->sk, &intrf->slaves[1].link, slave2_name);
	if (ret == -1) {
		DLOG_ERR("Slave2 is not exists");
		return ret;
	}
printf("61_change_interface_data\n");
	return 0;
}

int dict_interface_alloc(struct hsr_module *app,
			 struct hsr_interface **_interf,
			 const char *if_name,
			 const char *slave1_name,
			 const char *slave2_name)
{
	struct hsr_interface *interf;
	int ret;

	interf = (struct hsr_interface *)calloc(1, sizeof(struct hsr_interface));
	if (!interf) {
		DLOG_ERR("Unable to allocate memory for struct hsr_interface");
		return -1;
	}
printf("79_dict_interface_alloc\n");
	ret = change_interface_data(app, interf, if_name, slave1_name, slave2_name);
	if (ret < 0)
		return ret;

	interf->status = 0;
	interf->anode.key = interf->name;

	*_interf = interf;

	return 0;
}

void free_interface(struct hsr_interface *node)
{
	free(node->name);
	free(node->slaves[0].name);
	free(node->slaves[1].name);
	rtnl_link_put(node->slaves[0].link);
	rtnl_link_put(node->slaves[1].link);
	free(node);
}

void set_status_flags(struct hsr_interface *intrf, uint8_t flags)
{
	intrf->status |= flags;
}

void reset_status_flags(struct hsr_interface *intrf, uint8_t flags)
{
	intrf->status &= ~flags;
}

int add_interface(struct hsr_module *app, struct hsr_interface *intrf)
{
	int ret;

	ret = avl_insert(&app->interfaces, &intrf->anode);
	if (ret < 0) {
		DLOG_ERR("Key '%s' is already present", intrf->name);
		return ret;
	}
	return 0;
}

int add_new_interface(struct hsr_module *app,
		      struct hsr_interface **intrf,
		      const char *interface_name,
		      const char *slave1_name,
		      const char *slave2_name)
{
	struct hsr_interface *interf;
	int ret;
printf("131_add_new_interface\n");
	ret = dict_interface_alloc(app, &interf, interface_name,
				   slave1_name, slave2_name);
	if (ret < 0) {
		DLOG_ERR("Unable to allocate memory hsr_interface node in dictionary");
		return ret;
	}
	printf("138_add_new_interface\n");
	print_interface(interf);

	ret = add_interface(app, interf);
	if (ret < 0)
		return ret;

	*intrf = interf;

	return 0;
}

void remove_interfaces(struct hsr_module *app)
{
	struct hsr_interface *interface, *next_interface;

	avl_remove_all_elements(&app->interfaces, interface, anode, next_interface) {
		free_interface(interface);
	}
}

char *node_status2str(uint8_t status, char *buf, int len)
{
	uint8_t shift = 1;

	for (int i = len - 1; i > -1; i--) {
		if ((status & shift) > 0)
			buf[i] = '1';
		else
			buf[i] = '0';
		shift = shift << 1;
	}
	buf[len] = '\0';
	return buf;
}

void print_interface(struct hsr_interface *intrf)
{
	char str_status[6];

	printf("hsr_name = %s\n", intrf->name);
	printf("slave1 = %s\n", intrf->slaves[0].name);
	printf("slave2 = %s\n", intrf->slaves[1].name);
	printf("status = %s\n\n", node_status2str(intrf->status, str_status, 5));
}

void print_interfaces_dict(struct hsr_module *app)
{
	struct hsr_interface *intrf;
	int i = 0;

	printf("Interfaces:\n");
	avl_for_each_element(&app->interfaces, intrf, anode) {
		printf("interface %d:\n", i++);
		print_interface(intrf);
	}
}

int delete_interface_from_dict(struct hsr_module *app, const char *if_name)
{
	struct hsr_interface *interface, *next_interface;

	// list_for_each_entry_safe(node, next_node, &app->wait_list_head,
	// 			 list_node) {
	// 	if (!strcmp(node->hsr_name, if_name)) {
	// 		list_del(&node->list_node);
	// 		free_wait_list_node(node);
	// 		return 1;
	// 	}
	// }

	avl_for_each_element_safe(&app->interfaces, interface, anode,
				  next_interface) {
		if (!strcmp(interface->name, if_name)) {
			avl_delete(&app->interfaces, &interface->anode);
			free_interface(interface);
			return 1;
		}
	}

	return 0;
}

int add_interface_to_dict(struct hsr_module *app,
			  struct hsr_interface **intrf,
			  const char *interface_name,
			  const char *slave1_name,
			  const char *slave2_name)
{
	struct hsr_interface *interf;
	int ret;

	interf = avl_find_element(&app->interfaces, interface_name, interf, anode);
	printf("(interf == NULL) = %d\n", interf == NULL);
	if (!interf) {
		ret = add_new_interface(app, &interf, interface_name,
					slave1_name, slave2_name);
		if (ret < 0)
			return ret;
	} else {
		ret = change_interface_data(app, interf, interface_name,
					    slave1_name, slave2_name);
		return ret;
	}

	*intrf = interf;

	return 0;
}

int create_hif(struct hsr_interface *node)
{
	// int ret;

	// // printf("Attempt to create node:\n");
	// // print_wait_list_node(node);
	// ret = create_hif_from_waitlist(node->hsr_name, node->slave1_name,
	// 			       node->slave2_name);
	// if (ret < 0)
	// 	return ret;
	// list_del(&node->list_node);
	// free_wait_list_node(node);
	// return ret;
	return 0;
}

void new_link(struct hsr_module *app, struct rtnl_link *n_link)
{
	// //printf("\nnew_link\n");
	// struct wait_list_node *node, *next_node;
	// bool met = false;
	// char *link_name = rtnl_link_get_name(n_link);

	// list_for_each_entry_safe(node, next_node, &app->wait_list_head,
	// 			 list_node) {
	// 	if (!strcmp(node->slave1_name, link_name)) {
	// 		if (((node->status & SLAVE2_BUSY) == 0) &&
	// 		    ((node->status & SLAVE2_NOT_EXISTS) == 0)) {
	// 			if (!met &&
	// 			    ((node->status & HSR_STILL_EXISTS) == 0)) {
	// 				met = true;
	// 				create_hif(node);
	// 				break;
	// 			}
	// 		}

	// 	} else if (!strcmp(node->slave2_name, link_name)) {
	// 		if (((node->status & SLAVE1_BUSY) == 0) &&
	// 		    ((node->status & SLAVE1_NOT_EXISTS) == 0)) {
	// 			if (!met &&
	// 			    ((node->status & HSR_STILL_EXISTS) == 0)) {
	// 				met = true;
	// 				create_hif(node);
	// 				break;
	// 			}
	// 		}
	// 	}
	// }

	// list_for_each_entry(node, &app->wait_list_head, list_node) {
	// 	if (!strcmp(node->slave1_name, link_name))
	// 		reset_status_flags(node, SLAVE1_NOT_EXISTS);
	// 	else if (!strcmp(node->slave2_name, link_name))
	// 		reset_status_flags(node, SLAVE2_NOT_EXISTS);
	// }

	// print_wait_list(app);
}

void link_deleted(struct hsr_module *app, struct rtnl_link *o_link)
{
	// //printf("\nlink_deleted\n");
	// struct wait_list_node *node;
	// char *link_name = rtnl_link_get_name(o_link);

	// list_for_each_entry(node, &app->wait_list_head, list_node) {
	// 	if (!strcmp(node->slave1_name, link_name)) {
	// 		set_status_flags(node, SLAVE1_NOT_EXISTS);
	// 		reset_status_flags(node, SLAVE1_BUSY);
	// 	} else if (!strcmp(node->slave2_name, link_name)) {
	// 		set_status_flags(node, SLAVE2_NOT_EXISTS);
	// 		reset_status_flags(node, SLAVE2_BUSY);
	// 	}
	// }
	// print_wait_list(app);
}

void link_changed_released(struct hsr_module *app, struct rtnl_link *o_link)
{
	// //printf("\nlink_changed_released\n");
	// struct wait_list_node *node, *next_node;
	// char *link_name = rtnl_link_get_name(o_link);
	// bool met = false;

	// list_for_each_entry_safe(node, next_node, &app->wait_list_head,
	// 			 list_node) {
	// 	//находим ожидающий узел,
	// 	//который можно создать
	// 	if (!strcmp(node->slave1_name, link_name)) {
	// 		//если второй слэйв свободен
	// 		if ((node->status & SLAVE2_BUSY) == 0) {
	// 			//если мы ранее не создавали
	// 			//hsr интерфейса с таким link_name
	// 			if (!met) {
	// 				met = true;
	// 				//если интерфейс с таким
	// 				//именем в данный момент
	// 				//не удаляется
	// 				//создаем новый hsr интерфейс
	// 				if ((node->status &
	// 						HSR_STILL_EXISTS) == 0)
	// 					create_hif(node);
	// 				break;
	// 			}
	// 		}
	// 	} else if (!strcmp(node->slave2_name, link_name)) {
	// 		//если второй слэйв свободен
	// 		if ((node->status & SLAVE1_BUSY) == 0) {
	// 			//если мы ранее не создавали
	// 			//hsr интерфейса с таким link_name
	// 			if (!met) {
	// 				met = true;
	// 				if ((node->status &
	// 						HSR_STILL_EXISTS) == 0)
	// 					create_hif(node);
	// 				break;
	// 			}
	// 		}
	// 	}
	// }

	// if (!met)
	// 	list_for_each_entry(node, &app->wait_list_head, list_node) {
	// 		if (!strcmp(node->slave1_name, link_name))
	// 			reset_status_flags(node, SLAVE1_BUSY);
	// 		else if (!strcmp(node->slave2_name, link_name))
	// 			reset_status_flags(node, SLAVE2_BUSY);
	// 	}

	// //print_wait_list(app);
}

void link_changed_got_busy(struct hsr_module *app, struct rtnl_link *o_link)
{
	// //printf("\nlink_changed_got_busy\n");
	// struct wait_list_node *node;
	// char *link_name = rtnl_link_get_name(o_link);

	// list_for_each_entry(node, &app->wait_list_head, list_node) {
	// 	if (!strcmp(node->slave1_name, link_name))
	// 		set_status_flags(node, SLAVE1_BUSY);
	// 	else if (!strcmp(node->slave2_name, link_name))
	// 		set_status_flags(node, SLAVE2_BUSY);
	// }

	// //print_wait_list(app);
}

void link_released_hsr_not(struct hsr_module *app, struct rtnl_link *link,
			   struct rtnl_link *link_master)
{
	// //printf("\nlink_released_hsr_not\n");
	// struct wait_list_node *node;
	// char *link_name = rtnl_link_get_name(link);
	// char *master_link_name = rtnl_link_get_name(link_master);

	// list_for_each_entry(node, &app->wait_list_head, list_node) {
	// 	if (!strcmp(node->hsr_name, master_link_name))
	// 		set_status_flags(node, HSR_STILL_EXISTS);

	// 	if (!strcmp(node->slave1_name, link_name))
	// 		reset_status_flags(node, SLAVE1_BUSY);
	// 	else if (!strcmp(node->slave2_name, link_name))
	// 		reset_status_flags(node, SLAVE2_BUSY);
	// }

	// //print_wait_list(app);
}

void hsr_link_deleted(struct hsr_module *app, struct rtnl_link *hsr_link)
{
	// //printf("\nhsr_link_deleted\n");
	// struct wait_list_node *node, *next_node;
	// char *link_name = rtnl_link_get_name(hsr_link);
	// bool met = false;

	// list_for_each_entry_safe(node, next_node, &app->wait_list_head,
	// 			 list_node) {
	// 	if (!strcmp(node->hsr_name, link_name)) {
	// 		reset_status_flags(node, HSR_STILL_EXISTS);
	// 		if (((node->status & SLAVE1_BUSY) == 0) &&
	// 		    ((node->status & SLAVE2_BUSY) == 0) &&
	// 		    ((node->status & SLAVE2_NOT_EXISTS) == 0) &&
	// 		    ((node->status & SLAVE1_NOT_EXISTS) == 0)) {
	// 			met = true;
	// 			create_hif(node);
	// 			break;
	// 		}
	// 	}
	// }

	// if (!met)
	// 	list_for_each_entry_safe(node, next_node, &app->wait_list_head,
	// 				 list_node) {
	// 		if (((node->status & SLAVE1_BUSY) == 0) &&
	// 		    ((node->status & SLAVE2_BUSY) == 0) &&
	// 		    ((node->status & SLAVE2_NOT_EXISTS) == 0) &&
	// 		    ((node->status & SLAVE1_NOT_EXISTS) == 0)) {
	// 			met = true;
	// 			create_hif(node);
	// 			break;
	// 		}
	// 	}

	// //print_wait_list(app);
}
