#include "apply_conf.h"
#include <unistd.h>


int create_hsr_interface(const char *if_name, const uint32_t slave_1, 
                            const uint32_t slave_2, const int version) {

    struct rtnl_link *link;

	struct nl_sock *sk;
	int ret;

	sk = nl_socket_alloc();
	if ((ret = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}
	

	link = rtnl_link_hsr_alloc();

	rtnl_link_set_name(link, if_name);

	rtnl_link_hsr_set_version(link, version);

	rtnl_link_hsr_set_slave1(link, slave_1);

	rtnl_link_hsr_set_slave2(link, slave_2);

	rtnl_link_set_operstate(link, IF_OPER_UP);

	printf("\n46_rtnl_link_add\n");
	if ((ret = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) {
		//fprintf(stderr, "err = %d\n", err);
		nl_perror(ret, "Unable to add link");
		goto _error;
	}
	printf("\n52_rtnl_link_add\n");


	ret = 0;
_error:
	rtnl_link_put(link);
	nl_close(sk);

	return ret;
}


int delete_hsr_interface(const char *if_name) {

	struct rtnl_link *hsr_link;
	struct nl_sock *sk;
	struct nl_cache *link_cache;
	int err;

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}

	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		return err;
	}	

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
			
	err = rtnl_link_delete(sk, hsr_link);

	rtnl_link_put(hsr_link);
	nl_close(sk);

	return 0;
}


/*int change_hsr_interface(struct nl_sock *sk, struct rtnl_link *hsr_link, 
								struct rtnl_link *slave_1_link, struct rtnl_link *slave_2_link) {
		
		bool isChanges = false;
		int ret;
		
		struct rtnl_link *new_hsr_link = rtnl_link_hsr_alloc();
			
		rtnl_link_set_name(new_hsr_link, rtnl_link_get_name(hsr_link));
		
		rtnl_link_hsr_set_version(new_hsr_link, 1);

		int slave_1_id = rtnl_link_get_ifindex(slave_1_link);
		int slave_2_id = rtnl_link_get_ifindex(slave_2_link);
	
		int link_slave_1_id = rtnl_link_hsr_get_slave1(hsr_link);
		int link_slave_2_id = rtnl_link_hsr_get_slave2(hsr_link);


		printf("\ninput_slave1 = %d, input_slave2 = %d\nlink_slave1 = %d, link_slave2 = %d\n", 
			slave_1_id, slave_2_id, link_slave_1_id, link_slave_2_id);

		
		if ((slave_1_id == slave_2_id) || 
							((link_slave_1_id == slave_1_id) && (link_slave_2_id == slave_2_id))) {

			//изменения не требуются
			return 0;
		
		} else {
			
			printf("\n0\n");
		
			rtnl_link_hsr_set_slave2(new_hsr_link, slave_2_id);
			rtnl_link_hsr_set_slave1(new_hsr_link, slave_1_id);

			isChanges = true;

		}

		
		
		if (isChanges) {
			printf("\napply changes1\n");
			
			//ret = rtnl_link_change(sk, hsr_link, new_hsr_link, 0);
			ret = delete_hsr_interface(rtnl_link_get_name(hsr_link));

			ret = create_hsr_interface(rtnl_link_get_name(new_hsr_link), 
								rtnl_link_get_name(slave_1_link), rtnl_link_get_name(slave_2_link), 1);

			if (ret < 0) {
				fprintf(stderr, "\nerr = %d\n", ret);
				nl_perror(ret, "Unable to change link");
				
				return ret;
			
			} else {
				printf("\nret = %d\n", ret);
			}
		}
	
		rtnl_link_put(new_hsr_link);
		return 0;
		
}*/

int change_analysis(struct hsr_module *app, const char *if_name, 
								const char *slave_1_name, const char *slave_2_name) {

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
	printf("\n170_change_analysis rtnl_link_get_by_name %s\n", if_name);

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	
	if (hsr_link == NULL) {
	//printf("\n176_create_hsr_interface \n");
		ret = create_hsr_interface(if_name, rtnl_link_get_ifindex(slave_1_link), 
												rtnl_link_get_ifindex(slave_2_link), 1);
		if (ret < 0) {
			goto _error;

		}

		printf("\n180_create_hsr_interface created\n");
		sleep(5);
	}

	ret = add_interface_nodes_to_cache(app, if_name);
	if (ret < 0) {
		goto _error;
	}


	ret = 0;
_error:
	//printf("\n197_change_analysis  nl_cache_put\n");
	//nl_cache_put(link_cache);
	printf("\n199_change_analysis  nl_cache_put\n");
	nl_close(sk);
	return ret;

}

