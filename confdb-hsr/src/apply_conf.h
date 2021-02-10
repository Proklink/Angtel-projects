#include <stdio.h>
#include <stdbool.h>


#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/link/hsr.h>
#include <netlink/genl/hsr_node.h>
#include <netlink/utils.h>
#include <linux/netlink.h>


#include "status.h"

int create_hsr_interface(const char *if_name, const uint32_t slave_1, 
                            const uint32_t slave_2, const int version);

int delete_hsr_interface(const char *if_name);

// int change_hsr_interface(struct nl_sock *sk, struct rtnl_link *hsr_link, 
// 								struct rtnl_link *slave_1_link, struct rtnl_link *slave_2_link);

int change_analysis(const char *if_name, const char *slave_1_name, 
                            const char *slave_2_name);



