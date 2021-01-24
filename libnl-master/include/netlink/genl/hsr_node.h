
#include <linux/hsr_netlink.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hsr_node;


extern int hnode_alloc_cache_flags(struct nl_sock *sk, int family,
				struct nl_cache **result, unsigned int flags);

extern int hnode_info_parse(struct hsr_node *node, struct nlattr **tb);

extern int hnode_alloc_cache(struct nl_sock *sk, struct nl_cache **result);

extern void hnode_put(struct hsr_node *link);

extern struct hsr_node *hnode_alloc(void);

#ifdef __cplusplus
}
#endif


