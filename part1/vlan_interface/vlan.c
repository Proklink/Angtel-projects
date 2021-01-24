#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vlan.h>

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

	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {//выделяется кэш под интерфейс
		nl_perror(err, "Unable to allocate cache");
		return err;
	}

	if (!(master_index = rtnl_link_name2i(link_cache, "eth1"))) { //задаётся имя
		fprintf(stderr, "Unable to lookup eth0");
		return -1;
	}

	link = rtnl_link_vlan_alloc();//создаётся интерфейс

	rtnl_link_set_link(link, master_index); //интерфейсу присваивается индекс?

	rtnl_link_vlan_set_id(link, 10); //ещё один индекс?

	if ((err = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) { //запрос на создание интерфейса
		nl_perror(err, "Unable to add link");
		return err;
	}

	rtnl_link_put(link);//??
	nl_close(sk);

	return 0;
}
