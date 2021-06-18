#include "handlers.h"
#include <stdbool.h>
#include "wait_list.h"
#include "log.h"
#include "apply_conf.h"
#include "status.h"



void hnode_cache_change_cb(struct nl_cache *cache,
				       struct nl_object *o_obj,
				       struct nl_object *n_obj,
				       uint64_t attr_diff, int nl_act, void *data)
{
	struct hsr_module *app = data;

	if (app->interfaces_size > 0) {

		if (nl_act == NL_ACT_NEW) {
			printf("Adding\n");
			nl_object_dump(n_obj, &nl_debug_dp);
			add_node_to_list(app, n_obj);
			
		} else if (nl_act == NL_ACT_DEL) {
			printf("Deleting\n");
			nl_object_dump(o_obj, &nl_debug_dp);
			delete_node_from_list(app, o_obj);

		} else if (nl_act == NL_ACT_CHANGE) {
			struct nl_cache *mngr_hnode_cache = __nl_cache_mngt_require("hsr_node");

			nl_cache_add(mngr_hnode_cache, o_obj);
		}
	}

	
}


void link_changed_handler(struct hsr_module *app, struct nl_object *o_obj,struct nl_object *n_obj) {
	struct nl_dump_params nl_debug_dp = {
	.dp_type = NL_DUMP_LINE,
	.dp_fd = stdout,
	};
	printf("\nChanging (o_obj == NULL) = %d\n", o_obj == NULL);
	nl_object_dump(o_obj, &nl_debug_dp);
	printf("Changing (n_obj == NULL) = %d\n", n_obj == NULL);
	nl_object_dump(n_obj, &nl_debug_dp);

	char buf[128];
	struct rtnl_link *o_link = (struct rtnl_link *) o_obj;
	struct rtnl_link *n_link = (struct rtnl_link *) n_obj;
	char *link_type = nl_llproto2str(rtnl_link_get_arptype(o_link), buf, sizeof(buf));
	char *link_info_kind = rtnl_link_get_type(o_link);
			
	if (!link_info_kind)
		link_info_kind = "";

	struct nl_cache *cache = __nl_cache_mngt_require("route/link");
	struct rtnl_link *o_link_master = rtnl_link_get(cache, rtnl_link_get_master(o_link));
	struct rtnl_link *n_link_master = rtnl_link_get(cache, rtnl_link_get_master(n_link));

	if (!strcmp(link_type, "ether")) {
		if (strcmp(link_info_kind, "hsr")) {
				
			if ((o_link_master == NULL) && (n_link_master != NULL)) {
				link_changed_got_busy(app, o_link);
			}
			else if ((o_link_master != NULL) && (n_link_master == NULL)) {
				if (strcmp(rtnl_link_get_type(o_link_master), "hsr")) {
					printf("master: \n");
					nl_object_dump((struct nl_object *)o_link_master, &nl_debug_dp);
					link_changed_released(app, n_link);
				} else {
					link_released_hsr_not(app, n_link, o_link_master);
				}
			}
		} 
	}	
}

void link_deleted_handler(struct hsr_module *app, struct nl_object *o_obj) {
	struct nl_dump_params nl_debug_dp = {
	.dp_type = NL_DUMP_LINE,
	.dp_fd = stdout,
	};
	printf("\nDeleting (o_obj == NULL) = %d\n", o_obj == NULL);
	nl_object_dump(o_obj, &nl_debug_dp);
			
	char buf[128];
	struct rtnl_link *o_link = (struct rtnl_link *) o_obj;
	char *link_type = nl_llproto2str(rtnl_link_get_arptype(o_link), buf, sizeof(buf));
	char *link_info_kind = rtnl_link_get_type(o_link);
		
	if (!strcmp(link_type, "ether")) {
		if (strcmp(link_info_kind, "hsr")) {
			link_deleted(app, o_link);
		}
		else {
			hsr_link_deleted(app, o_link);
		}
	}
}

void link_new_handler(struct hsr_module *app, struct nl_object *o_obj,struct nl_object *n_obj) {
	struct nl_dump_params nl_debug_dp = {
	.dp_type = NL_DUMP_LINE,
	.dp_fd = stdout,
	};
	printf("\nAdding (n_obj == NULL) = %d\n", n_obj == NULL);
	nl_object_dump(n_obj, &nl_debug_dp);

	char buf[128];
	struct nl_cache *cache = __nl_cache_mngt_require("route/link");
	struct rtnl_link *n_link = (struct rtnl_link *) n_obj;
	char *link_type = nl_llproto2str(rtnl_link_get_arptype(n_link), buf, sizeof(buf));
	char *link_info_kind = rtnl_link_get_type(n_link);

	if (!strcmp(link_type, "ether")) {
		if (strcmp(link_info_kind, "hsr"))
			new_link(app, n_link);
		else {
			struct rtnl_link *slave = rtnl_link_get(cache, rtnl_link_get_link(n_link));

			link_changed_got_busy(app, slave);
		}
	}
}

void route_link_cache_change_cb(struct nl_cache *cache,
				       struct nl_object *o_obj,
				       struct nl_object *n_obj,
				       uint64_t attr_diff, int nl_act, void *data)
{
	struct hsr_module *app = data;
	
	if (nl_act == NL_ACT_NEW) 
		link_new_handler(app, o_obj, n_obj);
	 else if (nl_act == NL_ACT_DEL) 
		link_deleted_handler(app, o_obj);
	 else if (nl_act == NL_ACT_CHANGE) 
		link_changed_handler(app, o_obj, n_obj);
}

int hsr_handler(json_t *cdb_data, json_t *key, json_t *error, void *data)
{
	
	struct hsr_module *app = data;
	const char *interface_name;
	
	int ret;

	printf("\nstatus: %s\n", json_dumps(cdb_data, JSON_ENCODE_ANY));

	interface_name = json_string_value(key);
	if (interface_name == NULL) {
		DLOG_FUNC(STDLOG_ERR, "Unable to get interface name.");
		return -1;
	}

	if (cdb_data != NULL) {
		const char *slave_a = NULL, *slave_b = NULL;
		json_error_t json_error;
	

		/* Данные добавлены или изменены. */
		printf("Data added or changed: %s\n", interface_name);

		ret = change_interface_list(app, interface_name, 1);
		if (ret < 0) {
			DLOG_ERR("Failed to change interface list");
		}

		/* Извлечение добавленных или измененных данных. */
		ret = json_unpack_ex(cdb_data, &json_error, 0,
				     "{s:s, s:s}",
				     "slave-a", &slave_a,
				     "slave-b", &slave_b);
		if (ret == -1) {
			DLOG_FUNC(STDLOG_ERR,
				  "Error: %s. Source: %s. Line: %d. Column: %d.",
				  json_error.text, json_error.source,
				  json_error.line, json_error.column);
			return -1;
		}

		/* Здесь можно добавить какие-то полезные действия. */
		printf("slave-a: %s, slave-b: %s\n\n", slave_a, slave_b);

		ret = initial_check(app, interface_name, slave_a, slave_b);
		if (ret < 0) {
			return ret;
		}

	} else {
		/* Данные удалены. */
		printf("Data deleted: %s\n", interface_name);

		ret = delete_hsr_interface_by_name(interface_name);
		if (ret < 0) {
			DLOG_ERR("Failed to delete interface");

			ret = delete_wait_list_node(app, interface_name);
			if (!ret)
				DLOG_ERR("Hsr link %s not exists", interface_name);
			print_wait_list(app);
		}
		else {
			ret = change_interface_list(app, interface_name, 0);
			if (ret < 0) {
				DLOG_ERR("Failed to change interface list");
			}
		}

		

	}
    

	return 0;
}
