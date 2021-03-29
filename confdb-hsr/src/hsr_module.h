#include <confdb/confdb.h>
#include <confdb/log.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#define MODULE_NAME "hsr"

#define XPATH_ITF "/ietf-interfaces:interfaces/interface"


struct hsr_module {
	struct confdb *cdb;

	struct nl_sock *sk;
	struct nl_cache_mngr *nl_mngr;
	int cache_mngr_fd;

	int interfaces_size;
	uint32_t *interfaces;
	
};

int get_link_cache(struct nl_sock *sk,  struct nl_cache *link_cache);

void add_interface(struct hsr_module *app, uint32_t if_id);

void delete_interface(struct hsr_module *app, uint32_t if_id);

int find_interface(struct hsr_module *app,  uint32_t if_id);

int change_interface_list(struct hsr_module *app, const char *interface_name, int is_add);

