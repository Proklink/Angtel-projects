#include "status.h"
#include <jansson.h>

#include "log.h"

int add_node_to_list(struct hsr_module *app, struct nl_object *n_obj) {
	struct hsr_node *node = nl_object_priv(n_obj);
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	json_t *jv;
	int ret;
	char link_name[128];
    

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}
		
	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

    int ifindex = hnode_get_ifindex(node);

	if (!rtnl_link_i2name(link_cache, ifindex, link_name, sizeof(link_name))) {
        DLOG_ERR("No match was found");
		ret = -1;
		goto _error;
    }
	
	jv = json_pack("{s:[{ss}]}",  "node_list", "mac-address", hnode_get_mac_address_A(node));
	if (!jv) {
		DLOG_ERR("Failed to pack jv");
		ret = -1;
		goto _error;
	}
		
	ret = cdb_edit_status(app->cdb, CDB_OP_MERGE, jv,
		     XPATH_ITF"[name='%s']/angtel-hsr:hsr/node_list", 
			  link_name);
	if (ret < 0) {
		DLOG_ERR("Failed to cdb_edit_status");
		ret = -1;
		goto _error;
	}

	ret = 0;
_error:
	nl_cache_put(link_cache);
	nl_close(sk);

	return ret;
	
}

int delete_node_from_list(struct hsr_module *app, struct nl_object *n_obj) {
    struct hsr_node *node = nl_object_priv(n_obj);

	struct nl_cache *link_cache;
	struct nl_sock *sk;
	json_t *jv = NULL;
	int ret;
	char link_name[128];

    sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}
		
	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

    int ifindex = hnode_get_ifindex(node);

	if (!rtnl_link_i2name(link_cache, ifindex, link_name, sizeof(link_name))) {
        DLOG_ERR("No match was found");
		ret = -1;
		goto _error;
    }

    ret = cdb_edit_status(app->cdb, CDB_OP_REMOVE, jv,
		      XPATH_ITF "[name='%s']/angtel-hsr:hsr/node_list[mac-address='%s']", 
			  link_name, hnode_get_mac_address_A(node));
	if (ret < 0) {
		DLOG_ERR("Failed to cdb_edit_status");
		goto _error;
	}

	ret = 0;
_error:
	nl_cache_put(link_cache);
	nl_close(sk);


    return ret;
}

int add_interface_nodes_to_cache(struct nl_sock *sk, struct hsr_module *app, const char *if_name) {
	struct nl_sock *gen_sk;
	struct nl_object *hn_obj;
	struct rtnl_link *hsr_link;
	int ret;
	struct nl_cache *link_cache, *hnode_cache_fi;


	gen_sk = nl_socket_alloc();
	if ((ret = nl_connect(gen_sk, NETLINK_GENERIC)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);

	ret = hnode_alloc_cache_for_interface(gen_sk, &hnode_cache_fi, rtnl_link_get_ifindex(hsr_link));
	if (ret < 0)
		goto _error;
	
	struct nl_cache *mngr_hnode_cache = __nl_cache_mngt_require("hsr_node");


	for (NL_CACHE_ELEMENTS(hnode_cache_fi, hn_obj)) {
		struct hsr_node *node = nl_object_priv(hn_obj);

		struct hsr_node *snode = hnode_get_by_addr_ifindex(mngr_hnode_cache, 
										hnode_get_ifindex(node), hnode_get_mac_address_A(node));

		
		ret = add_node_to_list(app, hn_obj);
		if (ret < 0)
			goto _error;

		if (snode == NULL)
			nl_cache_add(mngr_hnode_cache, hn_obj);

	}


	
	ret = 0;
_error:
	nl_cache_put(link_cache);
	nl_cache_put(hnode_cache_fi);
	nl_close(gen_sk);

	return ret;
}