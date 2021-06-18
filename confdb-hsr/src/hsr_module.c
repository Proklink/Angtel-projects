#include "hsr_module.h"
#include "log.h" 
#include "handlers.h"
#include "wait_list.h"

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

/* Подписка отслеживание изменения данных. */
static struct cdb_data_notifier notifiers[] = {
	{
		/* Путь XPath в схеме по которому расположены отслеживаемые данные. */
		.schema_xpath = XPATH_ITF "/angtel-hsr:hsr",
		/* Обработчик, вызываемый при обнаружении изменения данных. */
		.handler = hsr_handler,
		/* Флаг, обозначающий, что изменение статусных данных нужно игнорировать. */
		.ignore_status = true
	},
	/* Признак завершения массива notifiers. */
	{
		.schema_xpath = NULL
	}
};

void app_destroy(struct hsr_module *app)
{
	if (!app)
		return;

	struct nl_cache *mngr_hnode_cache = __nl_cache_mngt_require("hsr_node");

	nl_cache_clear(mngr_hnode_cache);
		
	if (app->gen_manager->nl_mngr)
		nl_cache_mngr_free(app->gen_manager->nl_mngr);

	if (app->route_manager->nl_mngr)
		nl_cache_mngr_free(app->route_manager->nl_mngr);

	nl_socket_free(app->gen_manager->sk);
	nl_socket_free(app->route_manager->sk);

	free(app->interfaces);

	clean_wait_list(app);

	cdb_remove_extension(app->cdb, "hsr-app");
	cdb_wait_for_responses(app->cdb);
	cdb_destroy(app->cdb);
	free(app);
}

struct hsr_module *app_create(void)
{
	int ret;
	struct nl_cache *hnode_cache = NULL;
	struct nl_cache *link_cache = NULL;
	struct hsr_module *app = NULL;

	app = calloc(1, sizeof(*app));
	if (!app)
		return NULL;

	app->cdb = cdb_create();
	if (!app->cdb) {
		DLOG_ERR("Failed to create CDB client");
		goto on_error;
	}

	ret = cdb_add_extension(app->cdb, "hsr-app", NULL, notifiers, app);
	if (ret < 0) {
		DLOG_ERR("Failed to add extension on CDB client");
		goto on_error;
	}

	app->interfaces = NULL;
	app->interfaces_size = 0;

	app->is_init_stage = true;

	INIT_LIST_HEAD(&app->wait_list_head); 

	DLOG_INFO("Adding 'hsr_node' cache to cache manager");
	
	ret = hm_cache_manager_alloc(app, &app->gen_manager, NETLINK_GENERIC);
	if (ret < 0)
		goto on_error;
	
	ret = hnode_alloc_cache(app->gen_manager->sk, &hnode_cache);
	if (ret < 0) {
		DLOG_ERR("Failed to allocate 'hsr_node' cache: %s",
		     nl_geterror(ret));
		goto on_error;
	}
	
	ret = nl_cache_mngr_add_cache_v2(app->gen_manager->nl_mngr, hnode_cache, hnode_cache_change_cb, app);
	if (ret < 0) {
		DLOG_ERR("Failed to add cache to manager: %s",
			 nl_geterror(ret));
		nl_cache_free(hnode_cache);
		goto on_error;
	} 

	DLOG_INFO("Adding 'rtnl_link' cache to cache manager");
	
	ret = hm_cache_manager_alloc(app, &app->route_manager, NETLINK_ROUTE);
	if (ret < 0)
		goto on_error;
	
	ret = rtnl_link_alloc_cache_flags(NULL, AF_UNSPEC, &link_cache, NL_CACHE_AF_ITER);
	if (ret < 0) {
		DLOG_ERR("Failed to allocate 'route/link' cache: %s",
		     nl_geterror(ret));
		goto on_error;
	}
	
	ret = nl_cache_mngr_add_cache_v2(app->route_manager->nl_mngr, link_cache, route_link_cache_change_cb, app);
	if (ret < 0) {
		DLOG_ERR("Failed to add cache to manager: %s",
			 nl_geterror(ret));
		nl_cache_free(link_cache);
		goto on_error;
	} 

	return app;
on_error:
	app_destroy(app);
	return NULL;
}
