#include "apply_conf.h"
#include <unistd.h>


int create_hsr_interface(struct nl_sock *sk, const char *if_name, const uint32_t slave_1, 
                            const uint32_t slave_2, const int version) {

    struct rtnl_link *link;
	int ret;

	// struct nl_sock *sk;
	// sk = nl_socket_alloc();
	// if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
	// 	nl_perror(ret, "Unable to connect socket");
	// 	goto _error;
	// }

	if (sk == NULL) 
		return -1;

	link = rtnl_link_hsr_alloc();

	rtnl_link_set_name(link, if_name);

	rtnl_link_hsr_set_version(link, version);

	rtnl_link_hsr_set_slave1(link, slave_1);

	rtnl_link_hsr_set_slave2(link, slave_2);

	rtnl_link_set_operstate(link, IF_OPER_UP);

	if ((ret = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) {
		nl_perror(ret, "Unable to add link");
		goto _error;
	}

	ret = 0;
_error:
	rtnl_link_put(link);
	//nl_close(sk);

	return ret;
}


int delete_hsr_interface(const char *if_name) {

	struct rtnl_link *hsr_link = NULL;
	struct nl_sock *sk = NULL;
	struct nl_cache *link_cache = NULL;
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}	

	// ret = get_link_cache(sk, link_cache);
	// if (ret == -1) {
	// 	DLOG_ERR("error in socket allocation");
	// 	goto _error;
	// } else if (ret == -2) {
	// 	DLOG_ERR(" error in cache allocation");
	// 	goto _error;
	// }

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	if (!hsr_link) {
		ret = -1;
		goto _error;
	}
			
	ret = rtnl_link_delete(sk, hsr_link);
	if (ret < 0)
		goto _error;


	ret = 0;
_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	//printf("\n79_delete_hsr_interface  nl_cache_put sigfault?\n");
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;
}

int change_analysis(struct hsr_module *app, const char *if_name, 
								const char *slave_1_name, const char *slave_2_name) {
	printf("\n87_change_analysis\n");
	struct rtnl_link *hsr_link;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	struct rtnl_link *slave_1_link, *slave_2_link;
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	if ((ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	slave_1_link = rtnl_link_get_by_name(link_cache, slave_1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave_2_name);
		

	if ((slave_1_link == NULL) || (slave_2_link == NULL))
		goto _error;
	

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	
	if (hsr_link == NULL) {
		ret = create_hsr_interface(sk, if_name, rtnl_link_get_ifindex(slave_1_link), 
												rtnl_link_get_ifindex(slave_2_link), 1);
		if (ret < 0) {
			goto _error;

		}

		//printf("\n123_create_hsr_interface created\n");
		sleep(5);
	}

	ret = add_interface_nodes_to_cache(sk, app, if_name);
	if (ret < 0) {
		goto _error;
	}


	ret = 0;
_error:
	//printf("\n135_change_analysis  nl_cache_put sigfault?\n");
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;

}

