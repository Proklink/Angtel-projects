#ifndef WAIT_LIST_H
#define WAIT_LIST_H

#include "hsr_module.h"

#define SLAVE1_BUSY 		(1 << 0) 
#define SLAVE2_BUSY 		(1 << 1)
#define SLAVE1_NOT_EXISTS 	(1 << 2)
#define SLAVE2_NOT_EXISTS 	(1 << 3)
#define HSR_STILL_EXISTS 	(1 << 4) //для ограничения создания hsr интерфейса при существующем старом

struct wait_list_node {
	char *hsr_name;
	char *slave1_name;
	char *slave2_name;
	uint8_t status;  

	struct list_head list_node;
};

char *node_status2str(uint8_t status, char *buf, int len);

void print_wait_list(struct hsr_module *app);

void add_node_to_wait_list(struct wait_list_node *node, struct hsr_module *app);

void clean_wait_list(struct hsr_module *app);

void free_wait_list_node(struct wait_list_node *node);

void set_status_flags(struct wait_list_node *node, uint8_t flags);

int wait_list_node_alloc(struct wait_list_node **_node, 
						const char *if_name, 
						const char *slave_1_name, 
						const char *slave_2_name);

int delete_wait_list_node(struct hsr_module *app, const char *if_name);

void new_link(struct hsr_module *app, struct rtnl_link *n_link);

void link_deleted(struct hsr_module *app, struct rtnl_link *o_link);

void link_changed_released(struct hsr_module *app, struct rtnl_link *o_link);

void link_changed_got_busy(struct hsr_module *app, struct rtnl_link *o_link);

void link_released_hsr_not(struct hsr_module *app, 
							struct rtnl_link *link, 
							struct rtnl_link *link_master);

void hsr_link_deleted(struct hsr_module *app, struct rtnl_link *hsr_link);

#endif //WAIT_LIST_H