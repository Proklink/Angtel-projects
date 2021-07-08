#ifndef WAIT_LIST_H
#define WAIT_LIST_H

#include "hsr_module.h"

#define BIT(nr) (1 << (nr))

//Flags for the 'status' field of the hsr_interface structure

#define SLAVE1_BUSY			BIT(0)
#define SLAVE2_BUSY			BIT(1)
#define SLAVE1_NOT_EXISTS	BIT(2)
#define SLAVE2_NOT_EXISTS	BIT(3)
#define HSR_STILL_EXISTS	BIT(4)

//The "valid" field of the struct hsr_interface has the value true if the "status" field
//has the value 0.
//The "applied" field of the struct hsr_interface has the value true if host has hsr
//interface with the configuration in this struct hsr_interface.
struct hsr_interface {
	char *name;
	struct avl_node anode;

	bool valid;
	bool applied;
	uint8_t status;

	struct hsr_slave {
		char *name;
		struct rtnl_link *link;
	} slaves[2];
};

char *node_status2str(uint8_t status, char *buf, int len);

void print_interface(struct hsr_interface *intrf);

void print_interfaces_dict(struct hsr_module *app);

int add_interface(struct hsr_module *app, struct hsr_interface *intrf);

int add_new_interface(struct hsr_module *app,
		      struct hsr_interface **intrf,
		      const char *interface_name,
		      const char *slave1_name,
		      const char *slave2_name);

int add_interface_to_dict(struct hsr_module *app,
			  struct hsr_interface **intrf,
			  const char *interface_name,
			  const char *slave1_name,
			  const char *slave2_name);

void remove_interfaces(struct hsr_module *app);

void free_interface(struct hsr_interface *node);

void set_status_flags(struct hsr_interface *interf, uint8_t flags);

int change_interface_data(struct hsr_module *app,
			  struct hsr_interface *intrf,
			  const char *interface_name,
			  const char *slave1_name,
			  const char *slave2_name);

int dict_interface_alloc(struct hsr_module *app,
			 struct hsr_interface **_interf,
			 const char *if_name,
			 const char *slave1_name,
			 const char *slave2_name);

int delete_interface_from_dict(struct hsr_module *app, const char *if_name);

void new_link(struct hsr_module *app, struct rtnl_link *n_link);

void link_deleted(struct hsr_module *app, struct rtnl_link *o_link);

void link_changed_released(struct hsr_module *app, struct rtnl_link *o_link);

void link_changed_got_busy(struct hsr_module *app, struct rtnl_link *o_link);

void link_released_hsr_not(struct hsr_module *app,
			   struct rtnl_link *link,
			   struct rtnl_link *link_master);

void hsr_link_deleted(struct hsr_module *app, struct rtnl_link *hsr_link);

#endif //WAIT_LIST_H
