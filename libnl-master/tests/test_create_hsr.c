#include <stdio.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>

#include <netlink/route/link/hsr.h>

#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>


#include <linux/netlink.h>

int main(int argc, char *argv[])
{
	struct rtnl_link *link;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	int err, master_index;

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}

	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		return err;
	}

	
	link = rtnl_link_hsr_alloc();

	rtnl_link_set_name(link, "hsr0");

	rtnl_link_hsr_set_version(link, 1);


	if (!(master_index = rtnl_link_name2i(link_cache, "eth0"))) {
		fprintf(stderr, "Unable to lookup eth0");
		return -1;
	}

	rtnl_link_hsr_set_slave1(link, master_index);
	master_index = 0;


	if (!(master_index = rtnl_link_name2i(link_cache, "eth1"))) {
		fprintf(stderr, "Unable to lookup eth1");
		return -1;
	}

	rtnl_link_hsr_set_slave2(link, master_index);

	if ((err = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) {
		nl_perror(err, "Unable to add link");
		return err;
	}

	rtnl_link_put(link);
	nl_cache_put(link_cache);
	nl_close(sk);

	return 0;
}
