#include <netlink-private/netlink.h>
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink-private/route/link/api.h>
#include <netlink/route/link/hsr.h>


#define HSR_HAS_MULTICAST_SPEC		(1<<0)
#define HSR_HAS_SLAVE1_ID			(1<<1)
#define HSR_HAS_SLAVE2_ID			(1<<2)
#define HSR_HAS_VERSION				(1<<3)
#define HSR_HAS_SEQ_NR				(1<<4)
#define HSR_HAS_SUPERVISION_ADDR	(1<<5)

struct hsr_info
{
	uint8_t		vi_hsr_version;
	uint32_t		slave1_id;
	uint32_t		slave2_id;
	unsigned char 	multicast_spec;
	unsigned char 	supervision_addr[ETH_ALEN];
	uint16_t		seq_nr;
	uint32_t		vi_mask;
};


static struct nla_policy hsr_policy[IFLA_HSR_MAX+1] = {
	[IFLA_HSR_SLAVE1]				= { .type = NLA_U32 },
	[IFLA_HSR_SLAVE2]				= { .type = NLA_U32 },
	[IFLA_HSR_MULTICAST_SPEC]		= { .type = NLA_U8 },
	[IFLA_HSR_SEQ_NR]				= { .type = NLA_U16 },
	[IFLA_HSR_VERSION]				= { .type = NLA_U8 },
	[IFLA_HSR_SUPERVISION_ADDR]		= { .minlen = ETH_ALEN},
	
};

static int hsr_alloc(struct rtnl_link *link)
{
	struct hsr_info *vi;
	

	if (link->l_info) {
		memset(link->l_info, 0, sizeof(*vi));
	} else {
		if ((vi = calloc(1, sizeof(*vi))) == NULL)
			return -NLE_NOMEM;

		link->l_info = vi;
	}

	return 0;
}

static int hsr_parse(struct rtnl_link *link, struct nlattr *data,
		      struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_HSR_MAX+1];
	struct hsr_info *vi;
	int err;

	NL_DBG(3, "Parsing HSR link info\n");

	if ((err = nla_parse_nested(tb, IFLA_HSR_MAX, data, hsr_policy)) < 0)
		goto errout;

	if ((err = hsr_alloc(link)) < 0)
		goto errout;

	vi = link->l_info;

	if (tb[IFLA_HSR_SLAVE1]) {
		vi->slave1_id = nla_get_u32(tb[IFLA_HSR_SLAVE1]);
		vi->vi_mask |= HSR_HAS_SLAVE1_ID;
	}

	if (tb[IFLA_HSR_SLAVE2]) {
		vi->slave2_id = nla_get_u32(tb[IFLA_HSR_SLAVE2]);
		vi->vi_mask |= HSR_HAS_SLAVE2_ID;
	}

	if (tb[IFLA_HSR_VERSION]) {
		vi->vi_hsr_version = nla_get_u8(tb[IFLA_HSR_VERSION]);
		vi->vi_mask |= HSR_HAS_VERSION;
	}

	if (tb[IFLA_HSR_MULTICAST_SPEC]) {
		vi->multicast_spec = nla_get_u8(tb[IFLA_HSR_MULTICAST_SPEC]);
		vi->vi_mask |= HSR_HAS_MULTICAST_SPEC;
	}

	if (tb[IFLA_HSR_SUPERVISION_ADDR]) {
		nla_memcpy(vi->supervision_addr, tb[IFLA_HSR_SUPERVISION_ADDR], ETH_ALEN);
		//vi->supervision_addr = nla_get_u8(tb[IFLA_HSR_SUPERVISION_ADDR]);
		vi->vi_mask |= HSR_HAS_SUPERVISION_ADDR;
	}
	
	err = 0;
errout:
	return err;
}

static void hsr_free(struct rtnl_link *link)
{
	
	struct hsr_info *vi = link->l_info;
	if (link->l_info) {
		free(vi);
		link->l_info = NULL;
	}
	

}

static void hsr_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct hsr_info *vi = link->l_info;

	nl_dump(p, "hsr version %d", vi->vi_hsr_version);
}

static void hsr_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct hsr_info *vi = link->l_info;

	
	nl_dump_line(p, "    hsr-info name <%s>", rtnl_link_get_name(link));

	if (vi->vi_mask & HSR_HAS_VERSION)
		nl_dump_line(p, "	 hsr version %d ", vi->vi_hsr_version);

	nl_dump(p, "\n");

	if (vi->vi_mask & HSR_HAS_SLAVE1_ID)
		nl_dump_line(p, "    hsr slave1 id %d", vi->slave1_id);

	if (vi->vi_mask & HSR_HAS_SLAVE2_ID)
		nl_dump_line(p, ", hsr slave2 id %d ", vi->slave2_id);

	nl_dump(p, "\n");

	if (vi->vi_mask & HSR_HAS_MULTICAST_SPEC) {
		nl_dump_line(p, "	 hsr supervision %c ", vi->multicast_spec);

	nl_dump(p, "\n");
	}

	if (vi->vi_mask & HSR_HAS_SEQ_NR) { 
		nl_dump_line(p, "	 hsr sequence number %d ", vi->seq_nr);

	nl_dump(p, "\n");
	}

	if (vi->vi_mask & HSR_HAS_SUPERVISION_ADDR) 
		nl_dump_line(p, "	 hsr supervision addr %s ", vi->supervision_addr);

	nl_dump(p, "\n");
	
	
	}


static int hsr_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct hsr_info *vi = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	if (vi->vi_mask & HSR_HAS_SLAVE1_ID)
		NLA_PUT_U32(msg, IFLA_HSR_SLAVE1, vi->slave1_id);
		
	if (vi->vi_mask & HSR_HAS_SLAVE2_ID)
		NLA_PUT_U32(msg, IFLA_HSR_SLAVE2, vi->slave2_id);

	if (vi->vi_mask & HSR_HAS_VERSION)
		NLA_PUT_U8(msg, IFLA_HSR_VERSION, vi->vi_hsr_version);

	if (vi->vi_mask & HSR_HAS_MULTICAST_SPEC)
		NLA_PUT_U8(msg, IFLA_HSR_MULTICAST_SPEC, vi->multicast_spec);

	if (vi->vi_mask & HSR_HAS_SEQ_NR)
		NLA_PUT_U16(msg, IFLA_HSR_SEQ_NR, vi->seq_nr);	

	if (vi->vi_mask & HSR_HAS_SUPERVISION_ADDR)
		NLA_PUT(msg, IFLA_HSR_SUPERVISION_ADDR, ETH_ALEN, vi->supervision_addr);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static struct rtnl_link_info_ops hsr_info_ops = {
	.io_name		= "hsr",
	.io_alloc		= hsr_alloc,
	.io_parse		= hsr_parse,
	.io_dump = {
	    [NL_DUMP_LINE]	= hsr_dump_line,
	    [NL_DUMP_DETAILS]	= hsr_dump_details,
	},
	
	.io_put_attrs		= hsr_put_attrs,
	.io_free		= hsr_free,
};

/** @cond SKIP */
#define IS_HSR_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &hsr_info_ops) { \
		APPBUG("Link is not a hsr link. set type \"hsr\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

struct rtnl_link *rtnl_link_hsr_alloc(void)
{
	struct rtnl_link *link;
	int err;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if ((err = rtnl_link_set_type(link, "hsr")) < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}


int rtnl_link_is_hsr(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "hsr");
}

int rtnl_link_hsr_set_slave1(struct rtnl_link *link, uint32_t id)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	vi->slave1_id = id;
	vi->vi_mask |= HSR_HAS_SLAVE1_ID;

	return 0;
}

int rtnl_link_hsr_set_slave2(struct rtnl_link *link, uint32_t id)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	vi->slave2_id = id;
	vi->vi_mask |= HSR_HAS_SLAVE2_ID;

	return 0;
}

int rtnl_link_hsr_get_slave1(struct rtnl_link *link)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	return vi->slave1_id;
}

int rtnl_link_hsr_get_slave2(struct rtnl_link *link)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	return vi->slave2_id;
}


int hsr_is_set_slave1(struct rtnl_link *link) {

	struct hsr_info *vi = link->l_info;

	if (vi->slave1_id > 0)
		return 1;

	return 0;
	
}

int hsr_is_set_slave2(struct rtnl_link *link) {

	struct hsr_info *vi = link->l_info;

	if (vi->slave2_id > 0)
		return 1;

	return 0;
	
}

int rtnl_link_hsr_set_version(struct rtnl_link *link, uint16_t version)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	vi->vi_hsr_version = version;
	vi->vi_mask |= HSR_HAS_VERSION;

	return 0;
}

int rtnl_link_hsr_get_version(struct rtnl_link *link)
{
	struct hsr_info *vi = link->l_info;

	IS_HSR_LINK_ASSERT(link);

	if (vi->vi_mask & HSR_HAS_VERSION)
		return vi->vi_hsr_version;
	else
		return 0;
}


static void __init hsr_init(void)
{
	rtnl_link_register_info(&hsr_info_ops);
}

static void __exit hsr_exit(void)
{
	rtnl_link_unregister_info(&hsr_info_ops);
}


