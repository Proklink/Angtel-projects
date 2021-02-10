#include "status.h"



int fill_all(struct hsr_module *app) {
	printf("\n@dasd\n");
	
	struct nl_sock *sk;
	struct nl_object *obj;
	int err;
	struct nl_cache *link_cache;

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}

	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		return err;
	}

	for (NL_CACHE_ELEMENTS(link_cache, obj)) {
		struct rtnl_link *link = nl_object_priv(obj);

		if (!rtnl_link_is_hsr(link))
			continue;

		int err;
		int ifindex;
		
		ifindex = rtnl_link_get_ifindex(link);

		struct nl_cache *hnode_cache = __nl_cache_mngt_require("hsr_node");

		struct nl_object *hn_obj;

		for (NL_CACHE_ELEMENTS(hnode_cache, hn_obj)) {
			struct hsr_node *node = nl_object_priv(hn_obj);
			
			if (hnode_get_ifindex(node) != ifindex)
				continue;

			char *link_name = rtnl_link_get_name(link);
			printf("\n%s\n", link_name);
			json_t *jv;
			
			jv = json_pack("{s:[{ss}]}",  "node_list", "mac-address", hnode_get_mac_address_A(node));
			if (!jv) {
				DLOG_ERR("Failed to pack jv");
				return -1;
			}
		
			err = cdb_edit_status(app->cdb, CDB_OP_MERGE, jv,
					XPATH_ITF"[name='%s']/angtel-hsr:hsr/node_list", 
					link_name);
			if (err < 0) {
				DLOG_ERR("Failed to cdb_edit_status");
				return -1;
			}

		}

		
	}
	nl_close(sk);
	return 0;
}


int add_node_to_list(struct hsr_module *app, struct nl_object *n_obj) {
	struct hsr_node *node = nl_object_priv(n_obj);

	struct nl_cache *link_cache;
	struct nl_sock *sk;
	json_t *jv;
	int err;
	char link_name[128];
    

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}
		
	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		return err;
	}

    int ifindex = hnode_get_ifindex(node);

	if (!rtnl_link_i2name(link_cache, ifindex, link_name, sizeof(link_name))) {
        DLOG_ERR("No match was found");
		return -1;
    }


	jv = json_pack("{s:[{ss}]}",  "node_list", "mac-address", hnode_get_mac_address_A(node));
	if (!jv) {
		DLOG_ERR("Failed to pack jv");
		return -1;
	}
		
	err = cdb_edit_status(app->cdb, CDB_OP_MERGE, jv,
		     XPATH_ITF"[name='%s']/angtel-hsr:hsr/node_list", 
			  link_name);
	if (err < 0) {
		DLOG_ERR("Failed to cdb_edit_status");
		return -1;
	}

	return 0;
	
}

int delete_node_from_list(struct hsr_module *app, struct nl_object *n_obj) {

    struct hsr_node *node = nl_object_priv(n_obj);

	struct nl_cache *link_cache;
	struct nl_sock *sk;
	json_t *jv = NULL;
	int err;
	char link_name[128];

    sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}
		
	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		return err;
	}

    int ifindex = hnode_get_ifindex(node);

	if (!rtnl_link_i2name(link_cache, ifindex, link_name, sizeof(link_name))) {
        DLOG_ERR("No match was found");
		return -1;
    }


    err = cdb_edit_status(app->cdb, CDB_OP_REMOVE, jv,
		      XPATH_ITF "[name='%s']/angtel-hsr:hsr/node_list[mac-address='%s']", 
			  link_name, hnode_get_mac_address_A(node));
	if (err < 0) {
		DLOG_ERR("Failed to cdb_edit_status");
		return -1;
	}

    return 0;
}

int add_interface_to_cache(const char *if_name) {

	printf("\n161_add_interface_to_cache\n");

	struct nl_sock *sk, *gen_sk;
	struct nl_object *hn_obj;
	struct rtnl_link *hsr_link;
	int ret;
	struct nl_cache *link_cache, *hnode_cache;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		return ret;
	}

	gen_sk = nl_socket_alloc();
	if ((ret = nl_connect(gen_sk, NETLINK_GENERIC)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		return ret;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		return ret;
	}

	ret = hnode_alloc_cache(gen_sk, &hnode_cache);
	if (ret < 0)
		return ret;

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);

	for (NL_CACHE_ELEMENTS(hnode_cache, hn_obj)) {
		struct hsr_node *node = nl_object_priv(hn_obj);


		char buf[128];
		if ((hnode_get_ifindex(node) == rtnl_link_get_ifindex(hsr_link)) && 
				(hnode_get_mac_address_A(node) == nl_addr2str(rtnl_link_get_addr(hsr_link), buf, sizeof(buf)) )) {
				struct nl_cache *mngr_hnode_cache = __nl_cache_mngt_require("hsr_node");

				nl_cache_add(mngr_hnode_cache, hn_obj);
		}

	}

	
	nl_cache_put(link_cache);
	printf("\n208_link_cache\n");
	nl_cache_put(hnode_cache);
	printf("\n210_hnode_cache\n");
	nl_close(gen_sk);
	nl_close(sk);
	return 0;
}