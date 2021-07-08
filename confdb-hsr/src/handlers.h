#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdio.h>
#include "hsr_module.h"
#include <jansson.h>

int hsr_handler(json_t *cdb_data, json_t *key, json_t *error, void *data);

void hnode_cache_change_cb(struct nl_cache *cache,
				       struct nl_object *o_obj,
				       struct nl_object *n_obj,
				       uint64_t attr_diff, int nl_act, void *data);

void route_link_cache_change_cb(struct nl_cache *cache,
				       struct nl_object *o_obj,
				       struct nl_object *n_obj,
				       uint64_t attr_diff, int nl_act, void *data);

void link_changed_handler(struct hsr_module *app, struct nl_object *o_obj,struct nl_object *n_obj);

void link_deleted_handler(struct hsr_module *app, struct nl_object *o_obj);

void link_new_handler(struct hsr_module *app, struct nl_object *o_obj,struct nl_object *n_obj);



#endif //HANDLERS_H