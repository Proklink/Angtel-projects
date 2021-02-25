
#include <linux/hsr_netlink.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <sys/types.h>

#include <netlink/route/link/hsr.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hsr_node;


extern int hnode_alloc_cache_flags(struct nl_sock *sk, int family,
				struct nl_cache **result, unsigned int flags);

extern int hnode_info_parse(struct hsr_node *node, struct nlattr **tb);

extern int hnode_alloc_cache(struct nl_sock *sk, struct nl_cache **result);

extern int hnode_alloc_cache_for_interface(struct nl_sock *sk, struct nl_cache **result, int ifindex);
extern	struct hsr_node *hnode_get_by_addr_ifindex(struct nl_cache *cache, 
													const uint8_t ifindex, const char *addr);

extern void hnode_put(struct hsr_node *link);

extern int hnode_get_ifindex(struct hsr_node *node);

extern char *hnode_get_mac_address_A(struct hsr_node *node);

extern struct hsr_node *hnode_alloc(void);

#ifdef __cplusplus
}
#endif


