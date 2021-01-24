 #include <stdio.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
//#include <netlink/route/hsr_node.h>
#include <netlink/cache.h>
#include <netlink/route/link/hsr.h>
#include <netlink/genl/hsr_node.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>


#include <linux/netlink.h>

int main(int argc, char *argv[])
{
	struct nl_cache *nodes_cache;
	struct nl_sock *sk;
	int err;
	struct nl_dump_params dp = {
		.dp_type = NL_DUMP_DETAILS ,
		.dp_fd = stdout,
	};

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_GENERIC)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}
	
	if ((err = hnode_alloc_cache(sk, &nodes_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		printf("sigfault?\n");
		return err;
	}

	
	printf(" no sigfault\n");
	nl_cache_dump(nodes_cache, &dp);
	nl_close(sk);
  
	return 0;
}
