#ifndef HSR_MODULE_H
#define HSR_MODULE_H

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/link/hsr.h>
#include <netlink/genl/hsr_node.h>
#include <libubox/list.h>

#include <confdb/confdb.h>
#include <confdb/log.h>

#include <stdio.h>

#define MODULE_NAME "hsr"

#define XPATH_ITF "/ietf-interfaces:interfaces/interface"


struct hm_cache_manager {
	struct nl_sock *sk;
	struct nl_cache_mngr *nl_mngr;
	int cache_mngr_fd;
};

struct hsr_module {
	struct confdb *cdb;

	struct hm_cache_manager *gen_manager;
	struct hm_cache_manager *route_manager;

	int interfaces_size;
	uint32_t *interfaces;

	bool is_init_stage;
	
	struct list_head wait_list_head;
};



int hm_cache_manager_alloc(struct hsr_module *app, 
							struct hm_cache_manager **_hmcm,
                            int protocol);

int get_link_cache(struct nl_sock *sk,  struct nl_cache *link_cache);

void add_interface(struct hsr_module *app, uint32_t if_id);

void delete_interface(struct hsr_module *app, uint32_t if_id);

int find_interface(struct hsr_module *app,  uint32_t if_id);

int change_interface_list(struct hsr_module *app, const char *interface_name, int is_add);

struct hsr_module *app_create(void);

void app_destroy(struct hsr_module *app);

#endif //HSR_MODULE_H