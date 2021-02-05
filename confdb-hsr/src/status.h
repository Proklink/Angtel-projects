#include <jansson.h>
#include <netlink/route/link.h>
#include <netlink/route/link/hsr.h>

#include <netlink/genl/hsr_node.h>
#include "log.h"
#include "hsr_module.h"



int fill_all(struct hsr_module *app);

int add_node_to_list(struct hsr_module *app, struct nl_object *n_obj);

int delete_node_from_list(struct hsr_module *app, struct nl_object *n_obj);
