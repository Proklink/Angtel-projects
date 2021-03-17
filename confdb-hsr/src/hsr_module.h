#include <confdb/confdb.h>
#include <confdb/log.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>

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

