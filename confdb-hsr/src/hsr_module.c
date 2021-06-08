#include "hsr_module.h"
#include "log.h" 

void add_interface(struct hsr_module *app, uint32_t if_id) {
    app->interfaces_size++;
    app->interfaces = (uint32_t *)realloc(app->interfaces, (app->interfaces_size) * sizeof(uint32_t));
    app->interfaces[app->interfaces_size - 1] = if_id;

}

void delete_interface(struct hsr_module *app, uint32_t if_id) {

    app->interfaces = (uint32_t *)realloc(app->interfaces, (app->interfaces_size--) * sizeof(uint32_t));

}

int change_interface_list(struct hsr_module *app, const char *interface_name, int is_add) {
    uint32_t if_id;
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
    
    if ((if_id = rtnl_link_name2i(link_cache, interface_name)) < 0) {
        nl_perror(if_id, "No such link");
        goto _error;
    }

    if (!find_interface(app, if_id)) {
        if (is_add)
            add_interface(app, if_id);
        else
            delete_interface(app, if_id);
    }
    

    return 0;

_error:
    
    nl_cache_put(link_cache);
    nl_close(sk);

    return -1;

}

int find_interface(struct hsr_module *app,  uint32_t if_id) {

    for (int i = 0; i < app->interfaces_size; i++) {
        if (app->interfaces[i] == if_id)
            return 1;
    }

    return 0;

}


//-1 - error in socket allocation
//-2 - error in cache allocation
int get_link_cache(struct nl_sock *sk,  struct nl_cache *link_cache) {
    int ret;    

    sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		return -1;
	}
		
	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		return -2;
	}

    return 0;

}

int hm_cache_manager_alloc(struct hsr_module *app,
                            struct hm_cache_manager **_hmcm,
                            int protocol) {
    struct hm_cache_manager *hmcm = (struct hm_cache_manager *)calloc(1, sizeof(struct hm_cache_manager));
    int ret = 0;

    hmcm->sk = nl_socket_alloc();
	if (!hmcm->sk) {
		DLOG_ERR("Failed to alloc nl socket");
		goto on_error;
	}

	ret = nl_cache_mngr_alloc(hmcm->sk, protocol, NL_AUTO_PROVIDE, &hmcm->nl_mngr);
	if (ret < 0) {
		DLOG_ERR("Failed allocate cache manager: %s", nl_geterror(ret));
		goto on_error;
	}

	hmcm->cache_mngr_fd = nl_cache_mngr_get_fd(hmcm->nl_mngr);

    ret = nl_socket_set_buffer_size(hmcm->sk, 32*32768, 32*32768);
	if (ret < 0) {
		DLOG_ERR("Failed to set socket buffer size: %s",
			 nl_geterror(ret));
		goto on_error;
	}

    if (protocol == NETLINK_GENERIC) {
        nl_cache_mngr_refill_cache_on_adding(hmcm->nl_mngr, false);
    }
    
    *_hmcm = hmcm;

on_error:
    return ret;
}