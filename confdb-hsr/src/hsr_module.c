#include "hsr_module.h"


void add_interface(struct hsr_module *app, uint32_t if_id) {

    app->interfaces = (uint32_t *)realloc(app->interfaces, (app->interfaces_size++) * sizeof(uint32_t));
    app->interfaces [app->interfaces_size - 1] = if_id;

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