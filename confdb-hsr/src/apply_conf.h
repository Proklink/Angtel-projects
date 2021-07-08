#ifndef APPLY_CONF_H
#define APPLY_CONF_H

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/link/hsr.h>
#include <netlink/genl/hsr_node.h>
#include <netlink/utils.h>
#include <linux/netlink.h>
#include <linux/if.h>

#include "hsr_module.h"

int get_master(struct nl_sock *sk, int slave, struct rtnl_link **master);

int create_hsr_interface(struct nl_sock *sk,
			 char *if_name,
			 struct rtnl_link *slave_1,
			 struct rtnl_link *slave_2,
			 const int version);

int delete_hsr_interface(struct nl_sock *sk, struct rtnl_link *hsr_link);

int delete_hsr_interface_by_name(const char *if_name);

int recreate_hsr_interface(struct nl_sock *sk,
			   struct rtnl_link *old_hsr_link,
			   struct rtnl_link *slave_1,
			   struct rtnl_link *slave_2,
			   const int version);

int confdb_notification_processing(struct hsr_module *app,
				   const char *if_name,
				   const char *slave1_name,
				   const char *slave2_name);

int create_hif_from_waitlist(char *if_name, char *slave_1_name, char *slave_2_name);

#endif //APPLY_CONF_H
