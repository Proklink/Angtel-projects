#include "wait_list.h"
#include <string.h>
#include "apply_conf.h"


int wait_list_node_alloc(struct wait_list_node **_node, const char *if_name, 
                                            const char *slave_1_name, const char *slave_2_name) {
    struct wait_list_node * node = *_node;
    node = (struct wait_list_node *)calloc(1, sizeof(struct wait_list_node));
    if (node == NULL) {
        return -1;
    }

    node->hsr_name = strdup(if_name);
    node->slave1_name = strdup(slave_1_name);
    node->slave2_name = strdup(slave_2_name);

    node->status = 0;

    *_node = node;

    return 0;
    
}

void free_wait_list_node(struct wait_list_node *node) {
    free(node->hsr_name); 
    free(node->slave1_name); 
    free(node->slave2_name);
    free(node);
}

void set_status_flags(struct wait_list_node *node, uint8_t flags) {
    node->status |= flags;
}

void reset_status_flags(struct wait_list_node *node, uint8_t flags) {
    node->status &= ~flags;
}

void add_node_to_wait_list(struct wait_list_node *node, struct hsr_module *app) {
    list_add_tail(&node->list_node, &app->wait_list_head);
}

void clean_wait_list(struct hsr_module *app) {
    struct wait_list_node *node, *next_node;

    list_for_each_entry_safe(node, next_node, &app->wait_list_head, list_node) {
        list_del(&node->list_node);
        free_wait_list_node(node);
    }
}

char *node_status2str(uint8_t status, char *buf, int len) {
    uint8_t shift = 1;

    for (int i = len - 1; i > -1; i--) {
        if ((status & shift) > 0)
            buf[i] = '1';
        else
            buf[i] = '0';
        shift = shift << 1;
    }
    buf[len] = '\0';
    return buf;
}

void print_wait_list_node(struct wait_list_node *node) {
    char str_status[6];

    printf("hsr_name = %s\n", node->hsr_name);
    printf("slave1 = %s\n", node->slave1_name);
    printf("slave2 = %s\n", node->slave2_name);
    printf("status = %s\n\n", node_status2str(node->status, str_status, 5)); 
}

void print_wait_list(struct hsr_module *app) {
    struct wait_list_node *node;
    int i = 0;

    printf("Wait list:\n");
    list_for_each_entry(node, &app->wait_list_head, list_node) {
        printf("Node %d:\n", i++);
        print_wait_list_node(node);
    }
}

int delete_wait_list_node(struct hsr_module *app, const char *if_name) {
    struct wait_list_node *node, *next_node;

    list_for_each_entry_safe(node, next_node, &app->wait_list_head, list_node) {
        if (!strcmp(node->hsr_name, if_name)) {
            list_del(&node->list_node);
            free_wait_list_node(node);
            return 1;
        }
    }

    return 0;
}

int create_hif(struct wait_list_node *node) {
    int ret;

    printf("Attempt to create node:\n");
    print_wait_list_node(node);
    ret = create_hif_from_waitlist(node->hsr_name, node->slave1_name, node->slave2_name);
    if (ret < 0)
        return ret;
    list_del(&node->list_node);
    free_wait_list_node(node);
    printf("\n113_wait_list.c\n");
    return ret;

}

void new_link(struct hsr_module *app, struct rtnl_link *n_link) {
    printf("\nnew_link\n");
    struct wait_list_node *node, *next_node;
    bool met = false;
    char *link_name = rtnl_link_get_name(n_link);


    list_for_each_entry_safe(node, next_node, &app->wait_list_head, list_node) {
        if (!strcmp(node->slave1_name, link_name)) {
            if (((node->status & SLAVE2_BUSY) == 0) && ((node->status & SLAVE2_NOT_EXISTS) == 0)) {
                if ((met == false) && ((node->status & HSR_STILL_EXISTS) == 0)) {
                    met = true;
                    create_hif(node);
                    break;
                } 
            } 
            
        } else if (!strcmp(node->slave2_name, link_name)) {
            if (((node->status & SLAVE1_BUSY) == 0) && ((node->status & SLAVE1_NOT_EXISTS) == 0)) {
                if ((met == false) && ((node->status & HSR_STILL_EXISTS) == 0)) {
                    met = true;
                    create_hif(node);
                    break;
                } 
            }
        }
    }

    list_for_each_entry(node, &app->wait_list_head, list_node) {
        if (!strcmp(node->slave1_name, link_name)) {
            reset_status_flags(node, SLAVE1_NOT_EXISTS);

        } else if (!strcmp(node->slave2_name, link_name)) {

            reset_status_flags(node, SLAVE2_NOT_EXISTS);
        }
    }

    print_wait_list(app);
}

void link_deleted(struct hsr_module *app, struct rtnl_link *o_link) {
    printf("\nlink_deleted\n");
    struct wait_list_node *node;
    char *link_name = rtnl_link_get_name(o_link);

    list_for_each_entry(node, &app->wait_list_head, list_node) {
        if (!strcmp(node->slave1_name, link_name)) {
            set_status_flags(node, SLAVE1_NOT_EXISTS);
            reset_status_flags(node, SLAVE1_BUSY);
        } else if (!strcmp(node->slave2_name, link_name)) {
            set_status_flags(node, SLAVE2_NOT_EXISTS);
            reset_status_flags(node, SLAVE2_BUSY);
        }
    }
    print_wait_list(app);
}

void link_changed_released(struct hsr_module *app, struct rtnl_link *o_link) {
    printf("\nlink_changed_released\n");
    struct wait_list_node *node, *next_node;
    char *link_name = rtnl_link_get_name(o_link);
    bool met = false;

   list_for_each_entry_safe(node, next_node, &app->wait_list_head, list_node) {
        //находим ожидающий узел, который можно создать
        if (!strcmp(node->slave1_name, link_name)) {
            //если второй слэйв свободен
            if ((node->status & SLAVE2_BUSY) == 0) {
                //если мы ранее не создавали hsr интерфейса с таким link_name
                if (!met) {
                    met = true; 
                    //если интерфейс с таким именем в данный момент не удаляется
                    if ((node->status & HSR_STILL_EXISTS) == 0)
                        create_hif(node);//создаем новый hsr интерфейс
                    break;
                } 
            }
        } else if (!strcmp(node->slave2_name, link_name)) {
            //если второй слэйв свободен
            if ((node->status & SLAVE1_BUSY) == 0) {
                //если мы ранее не создавали hsr интерфейса с таким link_name
                if (!met) {
                    met = true; 
                    if ((node->status & HSR_STILL_EXISTS) == 0)
                        create_hif(node);
                    break;
                } 
            }
        }
    }

    if (!met)
        list_for_each_entry(node, &app->wait_list_head, list_node) {
            if (!strcmp(node->slave1_name, link_name)) {
                reset_status_flags(node, SLAVE1_BUSY);
            } else if (!strcmp(node->slave2_name, link_name)) {
                reset_status_flags(node, SLAVE2_BUSY);
            }
        }

    
    print_wait_list(app);
}

void link_changed_got_busy(struct hsr_module *app, struct rtnl_link *o_link) {
    printf("\nlink_changed_got_busy\n");
    struct wait_list_node *node;
    char *link_name = rtnl_link_get_name(o_link);


    list_for_each_entry(node, &app->wait_list_head, list_node) {
        if (!strcmp(node->slave1_name, link_name)) {
            set_status_flags(node, SLAVE1_BUSY);

        } else if (!strcmp(node->slave2_name, link_name)) {
            set_status_flags(node, SLAVE2_BUSY);
        }
    }

    print_wait_list(app);
}

void link_released_hsr_not(struct hsr_module *app, struct rtnl_link *link, struct rtnl_link *link_master) {
    printf("\nlink_released_hsr_not\n");
    struct wait_list_node *node;
    char *link_name = rtnl_link_get_name(link);
    char *master_link_name = rtnl_link_get_name(link_master);


    list_for_each_entry(node, &app->wait_list_head, list_node) {
        if (!strcmp(node->hsr_name, master_link_name)) {
            set_status_flags(node, HSR_STILL_EXISTS);
        }
        if (!strcmp(node->slave1_name, link_name)) {
            reset_status_flags(node, SLAVE1_BUSY);
        } else if (!strcmp(node->slave2_name, link_name)) {
            reset_status_flags(node, SLAVE2_BUSY);
        }
    }

    
    print_wait_list(app);
}

void hsr_link_deleted(struct hsr_module *app, struct rtnl_link *hsr_link) {
    printf("\nhsr_link_deleted\n");
    struct wait_list_node *node, *next_node;
    char *link_name = rtnl_link_get_name(hsr_link);
    bool met = false;
    //char *slave1_name = NULL;
    //char *slave2_name = NULL;

    list_for_each_entry_safe(node, next_node, &app->wait_list_head, list_node) {
        if (!strcmp(node->hsr_name, link_name))
            reset_status_flags(node, HSR_STILL_EXISTS);
        if (((node->status & SLAVE1_BUSY) == 0) && 
                ((node->status & SLAVE2_BUSY) == 0) && 
                ((node->status & SLAVE2_NOT_EXISTS) == 0) && 
                ((node->status & SLAVE1_NOT_EXISTS) == 0)) {
            if ((met == false) && ((node->status & HSR_STILL_EXISTS) == 0)) {
                met = true;
                //slave1_name = strdup(node->slave1_name);
                //slave2_name = strdup(node->slave2_name);
                create_hif(node);
                break;
            } 
        }
    }

    // list_for_each_entry(node, &app->wait_list_head, list_node) {
    //     if (!strcmp(node->slave1_name, slave1_name)) {
    //         if (met) 
    //             set_status_flags(node, SLAVE1_BUSY);
    //         reset_status_flags(node, SLAVE1_NOT_EXISTS);

    //     } else if (!strcmp(node->slave2_name, slave2_name)) {
    //         if (met) 
    //             set_status_flags(node, SLAVE2_BUSY);
    //         reset_status_flags(node, SLAVE2_NOT_EXISTS);
    //     }
    // }

    // printf("\n306_wait_list.c\n");
    // free(slave1_name);
    // free(slave2_name);
    // printf("\n309_wait_list.c\n");
    
    print_wait_list(app);
}

