#include "apply_conf.h"



int create_hsr_interface(const char *if_name, const char *slave_1, 
                            const char *slave_2, const int version) {

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

	rtnl_link_set_name(link, if_name);
	

	rtnl_link_hsr_set_version(link, version);
	

    if (!(master_index = rtnl_link_name2i(link_cache, slave_1))) {
		fprintf(stderr, "Unable to lookup eth0");
		return -1;
	}

	rtnl_link_hsr_set_slave1(link, master_index);
	master_index = 0;

	if (!(master_index = rtnl_link_name2i(link_cache, slave_2))) {
		fprintf(stderr, "Unable to lookup eth1");
		return -1;
	}

	rtnl_link_hsr_set_slave2(link, master_index);

	if ((err = rtnl_link_add(sk, link, NLM_F_CREATE)) < 0) {
		fprintf(stderr, "err = %d\n", err);
		nl_perror(err, "Unable to add link");
		return err;
	}

	rtnl_link_put(link);
	nl_close(sk);

	return 0;
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


int change_hsr_interface(struct nl_sock *sk, struct rtnl_link *hsr_link, 
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
		
}

int change_analysis(const char *if_name, const char *slave_1_name, const char *slave_2_name) {

	struct rtnl_link *hsr_link;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	struct rtnl_link *slave_1_link, *slave_2_link;
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

	slave_1_link = rtnl_link_get_by_name(link_cache, slave_1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave_2_name);
		
		if ((slave_1_link == NULL) || (slave_2_link == NULL))
			return -1;

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);


	if (hsr_link != NULL) {
		//hsr интерфейс существует, меняем слейвы
		printf("\n%s already exists\n", if_name);
		int ret = change_hsr_interface(sk, hsr_link, slave_1_link, slave_2_link);
		
		if (ret < 0)
			return -1;
		

	} else {
		bool isNewInterface = true;
		//struct nl_object *obj;
		
		
		// for (NL_CACHE_ELEMENTS(link_cache, obj)) {
			
		// 	struct rtnl_link *link = nl_object_priv(obj);
			
		// 	if (rtnl_link_is_hsr(link) != 0) {
		

		// 		int slave_1_id = rtnl_link_get_ifindex(slave_1_link);
		// 		int slave_2_id = rtnl_link_get_ifindex(slave_2_link);
		// 		int link_slave_1_id = rtnl_link_hsr_get_slave1(link);
		// 		int link_slave_2_id = rtnl_link_hsr_get_slave2(link);

		// 		if ( ( (link_slave_1_id == slave_1_id) && (link_slave_2_id == slave_2_id) ) || 
		// 			( (link_slave_1_id == slave_2_id) && (link_slave_2_id == slave_1_id) ) ) {
		// 				printf("\n%s already has this configuration\n", rtnl_link_get_name(link));
		// 				isNewInterface = false;
		// 		}
		// 	}

		// }

		if (isNewInterface) {
			err = create_hsr_interface(if_name, slave_1_name, slave_2_name, 1);
		
		}


	}

	return 0;

}