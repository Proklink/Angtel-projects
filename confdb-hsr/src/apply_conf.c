#include "apply_conf.h"
#include <unistd.h>
#include <stdbool.h>

#include "log.h"
#include "wait_list.h"

int get_sock_cache_route(struct nl_sock *sk, struct nl_cache *link_cache) {
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		return ret;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		return ret;
	}	

	return 1; 
}

int create_hsr_interface(struct nl_sock *sk, char *if_name, struct rtnl_link *slave_1, 
                        struct rtnl_link *slave_2, const int version) {
    struct rtnl_link *link;
	int ret;

	if (sk == NULL) 
		return -1;

	link = rtnl_link_hsr_alloc();

	rtnl_link_set_name(link, if_name);

	rtnl_link_hsr_set_version(link, version);

	rtnl_link_hsr_set_slave1(link, rtnl_link_get_ifindex(slave_1));

	rtnl_link_hsr_set_slave2(link, rtnl_link_get_ifindex(slave_2));

	if ((ret = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) {
		nl_perror(ret, "Unable to add link");
		goto _error;
	}

	ret = 0;
_error:
	rtnl_link_put(link);
	

	return ret;
}

int delete_hsr_interface_by_name(const char *if_name) {
	struct nl_cache *link_cache = NULL;
	struct rtnl_link *hsr_link = NULL;
	struct nl_sock *sk = NULL;
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}	

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	if (!hsr_link) {
		ret = -NLE_OBJ_NOTFOUND;
		nl_perror(ret, "Deleting failed");
		goto _error;
	}
			
	ret = rtnl_link_delete(sk, hsr_link);
	if (ret < 0)
		goto _error;


	ret = 0;
_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;
}


int delete_hsr_interface(struct rtnl_link *hsr_link, struct nl_sock *sk) {
	int ret;
			
	ret = rtnl_link_delete(sk, hsr_link);
	if (ret < 0) {
		if (ret == -NLE_OBJ_NOTFOUND)
			DLOG_ERR("No matching link exists for delete");
		else
			goto _error;
	}

	ret = 0;
_error:
	return ret;
}

int create_hif_from_waitlist(char *if_name, char *slave_1_name, char *slave_2_name) {
    struct rtnl_link *slave_1_link, *slave_2_link;
	struct rtnl_link *hsr_link = NULL;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}
	

	slave_1_link = rtnl_link_get_by_name(link_cache, slave_1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave_2_name);
	hsr_link = rtnl_link_get_by_name(link_cache, if_name);

	if (!hsr_link) {
		printf("\n135_apply_conf.c\n");
		ret = create_hsr_interface(sk, if_name, slave_1_link, slave_2_link, 1);printf("\n136_apply_conf.c\n");
		if (ret < 0)
			goto _error;
	} else {
				
		printf("\n141_apply_conf.c\n");
		ret = recreate_hsr_interface(sk, hsr_link, slave_2_link, slave_2_link, 1);printf("\n142_apply_conf.c\n");
		if (ret < 0)
			goto _error;
	}
	
_error: 
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;

}

int recreate_hsr_interface(struct nl_sock *sk, 
							struct rtnl_link *old_hsr_link, 
							struct rtnl_link *slave_1, 
							struct rtnl_link *slave_2, 
							int version) {
    int ret;

	ret = delete_hsr_interface(old_hsr_link, sk);
	if (ret < 0) {
		goto _error;
	}

	ret = create_hsr_interface(sk, rtnl_link_get_name(old_hsr_link), slave_1, slave_2, version);
	if (ret < 0) {
		goto _error;
	}

	ret = 0;
_error:

	return ret;
}

int hsr_recreation(struct hsr_module *app, 
					struct nl_sock *sk, 
					struct rtnl_link *old_hsr_link, 
					struct rtnl_link *slave_1, 
					struct rtnl_link *slave_2, 
					const int version) {
	int ret;

	set_unset_hsr_recreation(app, slave_1, slave_2);

	ret = recreate_hsr_interface(sk, old_hsr_link, slave_1, slave_2, version);
	if (ret < 0)
		return ret;

	set_unset_hsr_recreation(app, slave_1, slave_2);

	return 0;
}

int get_master(struct nl_sock *sk, int slave, struct rtnl_link **master) {
	struct nl_cache *link_cache;
	struct nl_object *hsr_obj;
	int ret;

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	for (NL_CACHE_ELEMENTS(link_cache, hsr_obj)) {
		struct rtnl_link *link = nl_object_priv(hsr_obj);

		if (!rtnl_link_is_hsr(link))
			continue;

		int link_slave1 = rtnl_link_hsr_get_slave1(link);
		int link_slave2 = rtnl_link_hsr_get_slave2(link);

		if ((slave == link_slave1) || (slave == link_slave2)) {
			*master = link;
			return 1;
		}
	}

	ret = 0;
_error:
	nl_cache_put(link_cache);

	return ret;
}

static int main_check(struct nl_sock *sk,
						struct hsr_module *app, 
						const char *if_name, 
						struct rtnl_link *slave_1_link, 
						struct rtnl_link *slave_2_link,
						struct rtnl_link *hsr_link, 
						struct wait_list_node *node) {

	struct rtnl_link *slave1_master = NULL;
	struct rtnl_link *slave2_master = NULL;
	int ret;

	if ((slave_1_link == NULL) || (slave_2_link == NULL)) {

		if ((slave_1_link == NULL) && (slave_2_link == NULL)) {
			set_status_flags(node, SLAVE1_NOT_EXISTS | SLAVE2_NOT_EXISTS);
			DLOG_ERR("Both link not exists");

		} else if (slave_1_link == NULL) {
			int slave2_ifindex = rtnl_link_get_ifindex(slave_2_link);

			set_status_flags(node, SLAVE1_NOT_EXISTS);
			get_master(sk, slave2_ifindex, &slave2_master);
			if (slave2_master != NULL)
				set_status_flags(node, SLAVE2_BUSY);
		
			DLOG_ERR("Slave1 not exists, slave2 exists");
		} else {
			int slave1_ifindex = rtnl_link_get_ifindex(slave_1_link);

			set_status_flags(node, SLAVE2_NOT_EXISTS);
			get_master(sk, slave1_ifindex, &slave1_master);
			if (slave1_master != NULL)
				set_status_flags(node, SLAVE1_BUSY);

			DLOG_ERR("Slave2 not exists, slave1 exists");
		}
		add_node_to_wait_list(node, app);
		
	} else {
		
		int slave1_ifindex = rtnl_link_get_ifindex(slave_1_link);
		int slave2_ifindex = rtnl_link_get_ifindex(slave_2_link);
		
		get_master(sk, slave1_ifindex, &slave1_master);
		get_master(sk, slave2_ifindex, &slave2_master);
		
		if (hsr_link) {
			int hlink_sl1 = rtnl_link_hsr_get_slave1(hsr_link);
			int hlink_sl2 = rtnl_link_hsr_get_slave2(hsr_link);

			if ( ((slave1_ifindex == hlink_sl1) && (slave2_ifindex == hlink_sl2)) || 
				 ((slave2_ifindex == hlink_sl1) && (slave1_ifindex == hlink_sl2)) ) {

				ret = hsr_recreation(app, sk, hsr_link, slave_1_link, slave_2_link, 1);
				if (ret < 0)
					goto _error;
				DLOG_ERR("Hsr link exists, slave1 and slave2 matches");
				free_wait_list_node(node);
			} else {
				if ((slave1_ifindex == hlink_sl1) || (slave1_ifindex == hlink_sl2)) {
					if (slave2_master == NULL) {
						ret = hsr_recreation(app, sk, hsr_link, slave_1_link, slave_2_link, 1);
						if (ret < 0)
							goto _error;
						DLOG_ERR("Hsr link exists, slave1 matches, slave2 exists");
						free_wait_list_node(node);
					} else {
						set_status_flags(node, SLAVE2_BUSY);
						add_node_to_wait_list(node, app);
						DLOG_ERR("Hsr link exists, slave1 matches, slave2 exists and busy");
					}	
				} else if ((slave2_ifindex == hlink_sl1) || (slave2_ifindex == hlink_sl2)) {
					if (slave1_master == NULL) {
						ret = hsr_recreation(app, sk, hsr_link, slave_1_link, slave_2_link, 1);
						if (ret < 0)
							goto _error;
						DLOG_ERR("Hsr link exists, slave2 matches, slave1 exists");
						free_wait_list_node(node);
					} else {
						set_status_flags(node, SLAVE1_BUSY);
						add_node_to_wait_list(node, app);
						DLOG_ERR("Hsr link exists, slave2 matches, slave1 exists and busy");
					}	
				} else {
					if ((slave1_master == NULL) && (slave2_master == NULL)) {
						ret = hsr_recreation(app, sk, hsr_link, slave_1_link, slave_2_link, 1);
						if (ret < 0)
							goto _error;
						DLOG_ERR("Hsr link exists, slave1 and slave2 exists");
						free_wait_list_node(node);
					} else if ((slave1_master != NULL) && (slave2_master != NULL)){
						set_status_flags(node, SLAVE1_BUSY | SLAVE2_BUSY);
						add_node_to_wait_list(node, app);
						DLOG_ERR("Hsr link exists, slave1 and slave2 exists and busy");
					}
				}
			}
			
		} else {
			if ((slave1_master == NULL) && (slave2_master == NULL)) {
				ret = create_hsr_interface(sk, (char *)if_name, slave_1_link, slave_2_link, 1);
				if (ret < 0)
					goto _error;
				free_wait_list_node(node);
				DLOG_ERR("Hsr link not exists, slave1 and slave2 exists and free\n");
			} else if ((slave1_master != NULL) && (slave2_master != NULL)) {
				set_status_flags(node, SLAVE1_BUSY | SLAVE2_BUSY);
				add_node_to_wait_list(node, app);
				DLOG_ERR("Hsr link not exists, slave1 and slave2 exists and busy\n");
				
			} else if ((slave1_master != NULL) && (slave2_master == NULL)) {
				set_status_flags(node, SLAVE1_BUSY);
				add_node_to_wait_list(node, app);
				DLOG_ERR("Hsr link not exists, slave1 busy, slave2 exists and free\n");
			} else if ((slave1_master == NULL) && (slave2_master != NULL)) {
				set_status_flags(node, SLAVE2_BUSY);
				add_node_to_wait_list(node, app);
				DLOG_ERR("Hsr link not exists, slave2 busy, slave1 exists and free\n");
			}
			
		}
	}

	ret = 0;
_error:
	return ret;
}

int initial_check(struct hsr_module *app, const char *if_name, 
								const char *slave_1_name, const char *slave_2_name) {
    struct rtnl_link *slave_1_link, *slave_2_link;
	struct wait_list_node *node = NULL;
	struct rtnl_link *hsr_link = NULL;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}
	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	slave_1_link = rtnl_link_get_by_name(link_cache, slave_1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave_2_name);
	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	
	delete_wait_list_node(app, if_name);

	ret = wait_list_node_alloc(&node, if_name, slave_1_name, slave_2_name);
	if (ret == -1) {
		DLOG_ERR("Failed to alloc wait list node");
		goto _error;
	}	

	ret = main_check(sk, app, if_name, slave_1_link, slave_2_link, hsr_link, node);
	if (ret < 0)
		goto _error;
	
	print_wait_list(app);

_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;
}