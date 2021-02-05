#include <netlink-private/netlink.h>
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/genl/hsr_node.h>
#include <netlink-private/utils.h>
#include <linux/hsr_netlink.h>
#include <netlink/genl/genl.h>
#include <netlink-private/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/mngt.h>
#include <netlink/msg.h>
#include <netlink/addr.h>
#include <netlink/attr.h>
#include <stdbool.h>


#define HNODE_ATTR_ADDR_A			(1 <<  0)
#define HNODE_ATTR_ADDR_B			(1 <<  1)
#define HNODE_ATTR_IFINDEX			(1 <<  2)
#define HNODE_ATTR_SLAVE1			(1 <<  3)
#define HNODE_ATTR_SLAVE2			(1 <<  4)
#define HNODE_ATTR_SLAVE1_AGE		(1 <<  5)
#define HNODE_ATTR_SLAVE2_AGE		(1 <<  6)
#define HNODE_ATTR_SLAVE1_SEQ		(1 <<  7)
#define HNODE_ATTR_SLAVE2_SEQ		(1 <<  8)
#define HNODE_ATTR_ADDR_B_IFINDEX	(1 <<  9)

#define HSR_VERSION		0x0001

#define GENL_ID_HSR 	31//0x1f

static struct nl_cache_ops hsr_node_ops;
static struct nl_object_ops hnode_obj_ops;
static uint32_t add_interface_ifindex;


struct nla_policy hnode_policy[HSR_A_MAX + 1] = {
	[HSR_A_IFINDEX] = { .type = NLA_U32 },
	[HSR_A_IF1_IFINDEX] = { .type = NLA_U32 },
	[HSR_A_IF2_IFINDEX] = { .type = NLA_U32 },
	[HSR_A_IF1_AGE] = { .type = NLA_U32 },
	[HSR_A_IF2_AGE] = { .type = NLA_U32 },
	[HSR_A_IF1_SEQ] = { .type = NLA_U16 },
	[HSR_A_IF2_SEQ] = { .type = NLA_U16 },
	[HSR_A_ADDR_B_IFINDEX] = { .type = NLA_U32 },
};

struct hsr_node *hnode_alloc(void)
{
	return (struct hsr_node *) nl_object_alloc(&hnode_obj_ops);
}

static void hnode_free_data(struct nl_object *c)
{
	struct hsr_node *node = nl_object_priv(c);

	if (node) {

		nl_addr_put(node->MAC_address_A);
		nl_addr_put(node->MAC_address_B);
		
	}
}

static int hnode_clone(struct nl_object *_dst, struct nl_object *_src)
{	
	struct hsr_node *dst = nl_object_priv(_dst);
	struct hsr_node *src = nl_object_priv(_src);


	if (src->MAC_address_A)
		if (!(dst->MAC_address_A = nl_addr_clone(src->MAC_address_A)))
			return -NLE_NOMEM;

	if (src->MAC_address_B)
		if (!(dst->MAC_address_B = nl_addr_clone(src->MAC_address_B)))
			return -NLE_NOMEM;

	if (src->hn_ifindex)
		if (!(dst->hn_ifindex = src->hn_ifindex))
			return -NLE_NOMEM;

	if (src->hn_slave1_ifindex)
		if (!(dst->hn_slave1_ifindex = src->hn_slave1_ifindex))
			return -NLE_NOMEM;
	
	if (src->hn_slave2_ifindex)
		if (!(dst->hn_slave2_ifindex = src->hn_slave2_ifindex))
			return -NLE_NOMEM;

	if (src->hn_slave1_age)
		if (!(dst->hn_slave1_age = src->hn_slave1_age))
			return -NLE_NOMEM;

	if (src->hn_slave2_age)
		if (!(dst->hn_slave2_age = src->hn_slave2_age))
			return -NLE_NOMEM;

	if (src->hn_slave1_seq)
		if (!(dst->hn_slave1_seq = src->hn_slave1_seq))
			return -NLE_NOMEM;

	if (src->hn_slave2_seq)
		if (!(dst->hn_slave2_seq = src->hn_slave2_seq))
			return -NLE_NOMEM;


	return 0;
}

static void hnode_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct hsr_node *node = (struct hsr_node *) obj;

	nl_dump(p, "HSR node index: ");

	if (node->ce_mask & HNODE_ATTR_IFINDEX)
		nl_dump(p, "%d", node->hn_ifindex);

	nl_dump(p, "\n");

	if (node->ce_mask & HNODE_ATTR_SLAVE1)
		nl_dump(p, "Slave 1 index %d", node->hn_slave1_ifindex);

	if (node->ce_mask & HNODE_ATTR_SLAVE2)
		nl_dump(p, ", slave 2 index %d", node->hn_slave2_ifindex);

	nl_dump(p, "\n");

}

int hnode_get_ifindex(struct hsr_node *node) {

	return node->hn_ifindex;

}

char *hnode_get_mac_address_A(struct hsr_node *node) {

	char buf[128];

	return nl_addr2str(node->MAC_address_A, buf, sizeof(buf));

}

static void hnode_dump_details(struct nl_object *obj, struct nl_dump_params *p)
{
	struct hsr_node *node = (struct hsr_node *) obj;
	char buf[128];
	
	hnode_dump_line(obj, p);

	if (node->MAC_address_A && !nl_addr_iszero(node->MAC_address_A))
		nl_dump(p, "MAC_address_A: %s", nl_addr2str(node->MAC_address_A, buf, sizeof(buf)));
	
	if (node->MAC_address_B && !nl_addr_iszero(node->MAC_address_B))
		nl_dump(p, ", MAC_address_B: %s", nl_addr2str(node->MAC_address_B, buf, sizeof(buf)));

	if (node->ce_mask & HNODE_ATTR_SLAVE1_SEQ)
		nl_dump(p, "\nSlave1 seq %d", node->hn_slave1_seq);

	if (node->ce_mask & HNODE_ATTR_SLAVE2_SEQ)
		nl_dump(p, ", slave2 seq %d", node->hn_slave2_seq);

	if (node->ce_mask & HNODE_ATTR_SLAVE1_AGE)
		nl_dump(p, "\nSlave1 age %d", node->hn_slave1_age);

	if (node->ce_mask & HNODE_ATTR_SLAVE2_AGE)
		nl_dump(p, ", slave2 age %d", node->hn_slave2_age);

	if (node->ce_mask & HNODE_ATTR_ADDR_B_IFINDEX)
		nl_dump(p, "\nAddr B index %d", node->hn_addr_B_ifindex);

	nl_dump(p, "\n");

}

static void hnode_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	hnode_dump_details(obj, p);
}

static uint32_t get_hsr_interface_index(char *name) {
	struct nl_cache *link_cache;
	struct rtnl_link *hsr_link;
	struct nl_sock *sock;
	int err;

	sock = nl_socket_alloc();
	if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}

	if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0)
		return err;


	if (!(hsr_link = rtnl_link_get_by_name(link_cache, name)))
		return -NLE_OBJ_NOTFOUND;



	rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);	
	nl_close(sock);

	return hsr_link->l_index;
}

/*static int get_hnode_status(struct nl_sock *sk, struct nl_cache *cache) {
	struct nl_object *obj;
	int err;
	struct nl_msg *msg = NULL;

	nl_list_for_each_entry(obj, &cache->c_items, ce_list) {
		struct hsr_node *node = (struct hsr_node *) obj;
		char buf[128];
		struct nl_dump_params dp = {
		.dp_type = NL_DUMP_DETAILS ,
		.dp_fd = stdout,
	};
		printf("iteration\n");

		msg = nlmsg_alloc();
		if (!msg)
			return -NLE_NOMEM;

		if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, GENL_ID_HSR,
				0, 0, HSR_C_GET_NODE_STATUS, HSR_VERSION)) {
			BUG();
			return -NLE_MSGSIZE;
		}

		err = nla_put_addr(msg, HSR_A_NODE_ADDR, node->MAC_address_A);
		if (err < 0)
			return err;
		nl_dump(&dp, "MAC_address_A: %s\n", nl_addr2str(node->MAC_address_A, buf, sizeof(buf)));


		err = nla_put_u32(msg, HSR_A_IFINDEX, 2);//get_hsr_interface_index("hsr0")
		if (err < 0)
			return err;

		err = nl_send_auto(sk, msg);
		if (err < 0)
			return err;

		err = nl_cache_pickup(sk, cache);
		if (err < 0)
			return err;	
	}

	return 0;
}*/

/*static int get_hnode_status_v2(struct hsr_node *node, struct nl_sock *sk, struct nl_cache *cache) {
		char buf[128];
		int err;
		struct nl_msg *msg = NULL;
		struct nl_dump_params dp = {
		.dp_type = NL_DUMP_DETAILS ,
		.dp_fd = stdout,
	};
		printf("get_hnode_status_v2\n");

		msg = nlmsg_alloc();
		if (!msg)
			return -NLE_NOMEM;

		if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, GENL_ID_HSR,
				0, 0, HSR_C_GET_NODE_STATUS, HSR_VERSION)) {
			BUG();
			return -NLE_MSGSIZE;
		}
		
		err = nla_put_addr(msg, HSR_A_NODE_ADDR, node->MAC_address_A);
		if (err < 0)
			return err;
		nl_dump(&dp, "MAC_address_A: %s\n", nl_addr2str(node->MAC_address_A, buf, sizeof(buf)));

		err = nla_put_u32(msg, HSR_A_IFINDEX, node->hn_ifindex);//get_hsr_interface_index("hsr0")
		if (err < 0)
			return err;

		err = nl_send_auto(sk, msg);
		if (err < 0)
			return err;

		err = nl_cache_pickup(sk, cache);
		if (err < 0)
			return err;	

		return 0;
}
*/

/*int hnode_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	int ret;
	struct nl_sock *sock;
	struct nl_cache *link_cache;
	struct rtnl_link *link;
	
	int size = 0;

	add_interface_ifindex = -1;
	
	int *added_interfaces = NULL;
	
	sock = nl_socket_alloc();
	if ((ret = nl_connect(sock, NETLINK_ROUTE)) < 0) {
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0)
		goto _error;

	if (!(*result = nl_cache_alloc(&hsr_node_ops))) {
		ret = -NLE_NOMEM;
		goto _error;
	}

	nl_list_for_each_entry(link, &link_cache->c_items, ce_list) {
	
		if (!rtnl_link_is_hsr(link))
			continue;
		
		bool isContinue = false;

		if (added_interfaces != NULL)
			for (int i = 0; i < size; i++) {
				if (rtnl_link_get_ifindex(link) == added_interfaces[i]) {
					isContinue = true;
					break;
				}

			}
		

		if (isContinue) {
			continue;
		
		} else {

			add_interface_ifindex = rtnl_link_get_ifindex(link);

			struct nl_cache *temp_result = NULL;

			ret = nl_cache_alloc_and_fill(&hsr_node_ops, sk, &temp_result);
			if (ret < 0)
				goto _error;

			if (temp_result == NULL)
				goto _error;
		
			added_interfaces = (int *)realloc(added_interfaces, (size++) * sizeof(int));
			added_interfaces[size-1] = add_interface_ifindex;

			struct nl_object *obj;
			
			nl_list_for_each_entry(obj, &temp_result->c_items, ce_list) {
				ret = nl_cache_add(*result, obj);
				if (ret < 0)
					goto _error;
				
			}
		}
		
	}
	
	printf("\nno error in hnode_alloc_cache\n");
_error:
	
	//nl_cache_put(link_cache);
	nl_close(sock);
	free(added_interfaces);
	return ret;	
}*/

int hnode_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	
	int ret = nl_cache_alloc_and_fill(&hsr_node_ops, sk, result);
	if (ret < 0)
		return ret;

	return 0;	
}


static int hnode_request_update(struct nl_cache *c, struct nl_sock *h)
{
	int err;
	struct nl_msg *msg = NULL;

	//создать сообщение
	msg = nlmsg_alloc();
	if (!msg)
		return -NLE_NOMEM;

	//заголовок добавить к сообщению
	if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, GENL_ID_HSR,
			0, 0, HSR_C_GET_NODE_LIST, HSR_VERSION)) {
		BUG();
		return -NLE_MSGSIZE;
	}

	//добавить атрибут
	err = nla_put_u32(msg, HSR_A_IFINDEX, get_hsr_interface_index("hsr0"));
	if (err < 0)
		return err;


	//отправить сообщение
	err = nl_send_auto(h, msg);
	if (err < 0)
		return err;
	
		
	return 0;
}

int hnode_info_parse(struct hsr_node *node, struct nlattr **tb)
{

	if (tb[HSR_A_NODE_ADDR]) {
		printf("HSR_A_NODE_ADDR\n");
		node->MAC_address_A = nl_addr_alloc_attr(tb[HSR_A_NODE_ADDR], AF_UNSPEC);
		if (node->MAC_address_A == NULL)
			return -NLE_NOMEM;

		nl_addr_set_family(node->MAC_address_A,
				   nl_addr_guess_family(node->MAC_address_A));
		node->ce_mask |= HNODE_ATTR_ADDR_A;
	}

	if (tb[HSR_A_NODE_ADDR_B]) {
		printf("HSR_A_NODE_ADDR_B\n");
		node->MAC_address_B = nl_addr_alloc_attr(tb[HSR_A_NODE_ADDR_B], AF_UNSPEC);
		if (node->MAC_address_B == NULL)
			return -NLE_NOMEM;

		nl_addr_set_family(node->MAC_address_B,
				   nl_addr_guess_family(node->MAC_address_B));
		node->ce_mask |= HNODE_ATTR_ADDR_B;
	}

	if (tb[HSR_A_IFINDEX]) {
		printf("HSR_A_IFINDEX\n");
		node->hn_ifindex = nla_get_u32(tb[HSR_A_IFINDEX]);
		node->ce_mask |= HNODE_ATTR_IFINDEX;
		printf("node->hn_ifindex = %d\n", node->hn_ifindex);
	}

	if (tb[HSR_A_IF1_IFINDEX]) {
		printf("HSR_A_IF1_IFINDEX\n");
		node->hn_ifindex = nla_get_u32(tb[HSR_A_IF1_IFINDEX]);
		node->ce_mask |= HNODE_ATTR_SLAVE1;
	}

	if (tb[HSR_A_IF2_IFINDEX]) {
		printf("HSR_A_IF2_IFINDEX\n");
		node->hn_ifindex = nla_get_u32(tb[HSR_A_IF2_IFINDEX]);
		node->ce_mask |= HNODE_ATTR_SLAVE2;
	}

	if (tb[HSR_A_IF1_AGE]) {
		printf("HSR_A_IF1_AGE\n");
		node->hn_slave1_age = nla_get_u32(tb[HSR_A_IF1_AGE]);
		node->ce_mask |= HNODE_ATTR_SLAVE1_AGE;
	}

	if (tb[HSR_A_IF2_AGE]) {
		printf("HSR_A_IF2_AGE\n");
		node->hn_slave2_age = nla_get_u32(tb[HSR_A_IF2_AGE]);
		node->ce_mask |= HNODE_ATTR_SLAVE2_AGE;
	}

	if (tb[HSR_A_IF1_SEQ]) {
		printf("HSR_A_IF1_SEQ\n");
		node->hn_slave1_seq = nla_get_u16(tb[HSR_A_IF1_SEQ]);
		node->ce_mask |= HNODE_ATTR_SLAVE1_SEQ;
	}

	if (tb[HSR_A_IF2_SEQ]) {
		printf("HSR_A_IF2_SEQ\n");
		node->hn_slave1_seq = nla_get_u16(tb[HSR_A_IF2_SEQ]);
		node->ce_mask |= HNODE_ATTR_SLAVE2_SEQ;
	}

	if (tb[HSR_A_ADDR_B_IFINDEX]) {
		printf("HSR_A_ADDR_B_IFINDEX\n");
		node->hn_addr_B_ifindex = nla_get_u32(tb[HSR_A_ADDR_B_IFINDEX]);
		node->ce_mask |= HNODE_ATTR_ADDR_B_IFINDEX;
	}

	return 0;
}


static int hnode_msg_node_list_parser(struct nl_cache_ops *ops, struct genl_cmd *cmd,
			   												struct genl_info *info, void *arg)
{	
	printf("\nhnode_msg_node_list_parser\n");
	if (info->attrs[HSR_A_IFINDEX] == NULL) 
		return -NLE_MISSING_ATTR;
	struct nl_parser_param *pp = arg;

	int maxtype = cmd->c_maxattr;
	struct nlattr *head = nlmsg_attrdata(info->nlh, GENL_HDRSIZE(ops->co_genl->o_hdrsize));
	int len = nlmsg_attrlen(info->nlh, GENL_HDRSIZE(ops->co_genl->o_hdrsize));  
	
	struct nlattr *nla;
	int rem, err;

	uint32_t ifindex = nla_get_u32(info->attrs[HSR_A_IFINDEX]);
	
	nla_for_each_attr(nla, head, len, rem) {
		int type = nla_type(nla);
		if (type > maxtype)
			continue;

		struct hsr_node *node;

		if (nla->nla_type == HSR_A_NODE_ADDR) {
			node = hnode_alloc();
			if (node == NULL) 
				return -NLE_NOMEM;

			node->ce_msgtype = info->nlh->nlmsg_type;

			node->MAC_address_A = nl_addr_alloc_attr(nla, AF_UNSPEC);
			if (node->MAC_address_A == NULL)
				return -NLE_NOMEM;

			nl_addr_set_family(node->MAC_address_A, nl_addr_guess_family(node->MAC_address_A));
			node->ce_mask |= HNODE_ATTR_ADDR_A;
			
			node->hn_ifindex = ifindex;
			node->ce_mask |= HNODE_ATTR_IFINDEX;
			
			err = pp->pp_cb((struct nl_object *) node, pp);
			if (err < 0)
				return err;
			}
	}

	return 0;
}

static int hnode_msg_node_status_parser(struct nl_cache_ops *ops, struct genl_cmd *cmd,
			   												struct genl_info *info, void *arg)
{
	struct hsr_node *node;
	struct nl_parser_param *pp = arg;
	int err;
	printf("hnode_msg_node_status_parser\n");
	if (info->attrs[HSR_A_IFINDEX] == NULL) 
		return -NLE_MISSING_ATTR;

	node = hnode_alloc();
	if (node == NULL) 
		return -NLE_NOMEM;

	node->ce_msgtype = info->nlh->nlmsg_type;

	err = hnode_info_parse(node, info->attrs);
	if (err < 0)
		return err;

	return pp->pp_cb((struct nl_object *) node, pp);
}

void hnode_put(struct hsr_node *link)
{
	nl_object_put((struct nl_object *) link);
}

static struct genl_cmd genl_cmds[] = {
	{
		.c_id		= HSR_C_SET_NODE_LIST,
		.c_name		= "SET_NODE_LIST",
		.c_maxattr	= HSR_A_MAX,
		.c_attr_policy	= hnode_policy,
		.c_msg_parser	= hnode_msg_node_list_parser,
	},
	{
		.c_id		= HSR_C_NODE_DOWN,
		.c_name		= "NODE_DOWN" ,
	},
	{
		.c_id		= HSR_C_GET_NODE_STATUS,
		.c_name		= "GET_NODE_STATUS" ,
	},
	{
		.c_id		= HSR_C_SET_NODE_STATUS,
		.c_name		= "SET_NODE_STATUS" ,
		.c_maxattr	= HSR_A_MAX,
		.c_attr_policy	= hnode_policy,
		.c_msg_parser	= hnode_msg_node_status_parser,
	},
	{
		.c_id		= HSR_C_GET_NODE_LIST,
		.c_name		= "GET_NODE_LIST" ,
	},
};


static struct genl_ops genl_ops = {
	.o_cmds			= genl_cmds,
	.o_ncmds		= ARRAY_SIZE(genl_cmds),
};

static struct nl_object_ops hnode_obj_ops = {
	.oo_name		= "hsr_node",
	.oo_size		= sizeof(struct hsr_node),
	.oo_free_data		= hnode_free_data,
	.oo_clone		= hnode_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= hnode_dump_line,
	    [NL_DUMP_DETAILS]	= hnode_dump_details,
		[NL_DUMP_STATS]	= hnode_dump_stats,
	},
};

static struct nl_af_group hnode_groups[] = {
	{ AF_UNSPEC,	0x0b },
	{ END_OF_GROUP_LIST },
};


static struct nl_cache_ops hsr_node_ops = {
	.co_name		= "hsr_node",
	.co_hdrsize		= GENL_HDRSIZE(0),
	.co_msgtypes		= GENL_FAMILY(GENL_ID_HSR, "HSR"),
	.co_genl		= &genl_ops,
	.co_groups		= hnode_groups,
	.co_protocol		= NETLINK_GENERIC,
	.co_request_update	= hnode_request_update, 
	.co_obj_ops		= &hnode_obj_ops,
};

static void __init hnode_init(void)
{
	genl_register(&hsr_node_ops);
}

static void __exit hnode_exit(void)
{
	genl_unregister(&hsr_node_ops);
}