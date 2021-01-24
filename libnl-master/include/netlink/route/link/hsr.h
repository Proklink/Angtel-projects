#ifndef NETLINK_LINK_HSR_H_
#define NETLINK_LINK_HSR_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

/*struct vlan_map
{
	uint32_t		vm_from;
	uint32_t		vm_to;
};*/

//#define VLAN_PRIO_MAX 7

extern struct rtnl_link *rtnl_link_hsr_alloc(void);

extern int	rtnl_link_is_hsr(struct rtnl_link *);
extern int 	rtnl_link_hsr_set_slave1(struct rtnl_link *link, uint32_t id);
extern int 	rtnl_link_hsr_set_slave2(struct rtnl_link *link, uint32_t id);
extern int 	rtnl_link_hsr_get_slave1(struct rtnl_link *link);
extern int 	rtnl_link_hsr_get_slave2(struct rtnl_link *link);
extern int 	hsr_is_set_slave1(struct rtnl_link *link);
extern int 	hsr_is_set_slave2(struct rtnl_link *link);

extern int			 rtnl_link_hsr_get_version(struct rtnl_link *link);
extern int			rtnl_link_hsr_set_version(struct rtnl_link *link, uint16_t version);

#ifdef __cplusplus
}
#endif

#endif
