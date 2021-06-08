#ifndef STATUS_H
#define STATUS_H

#include <netlink/route/link.h>
#include <netlink/route/link/hsr.h>
#include <netlink/addr.h>
#include <netlink/cache.h>
#include <netlink/genl/hsr_node.h>

#include "hsr_module.h"

int add_node_to_list(struct hsr_module *app, struct nl_object *n_obj);

int delete_node_from_list(struct hsr_module *app, struct nl_object *n_obj);

int add_interface_nodes_to_cache(struct nl_sock *sk, struct hsr_module *app, const char *if_name);

#endif //STATUS_H