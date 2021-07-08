#include <unistd.h>
#include <stdbool.h>
#include "apply_conf.h"
#include "log.h"
#include "wait_list.h"
#include <libubox/avl.h>

int get_sock_cache_route(struct nl_sock *sk, struct nl_cache *link_cache)
{
	int ret;

	sk = nl_socket_alloc();
	ret = nl_connect(sk, NETLINK_ROUTE);
	if (ret < 0) {
		nl_perror(ret, "Unable to connect socket");
		return ret;
	}

	ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		return ret;
	}

	return 1;
}

int create_hsr_interface(struct nl_sock *sk, char *if_name,
			 struct rtnl_link *slave_1, struct rtnl_link *slave_2,
			 const int version)
{
	struct rtnl_link *link;
	int ret;

	if (!sk)
		return -1;

	link = rtnl_link_hsr_alloc();

	rtnl_link_set_name(link, if_name);

	rtnl_link_hsr_set_version(link, version);

	rtnl_link_hsr_set_slave1(link, rtnl_link_get_ifindex(slave_1));

	rtnl_link_hsr_set_slave2(link, rtnl_link_get_ifindex(slave_2));

	ret = rtnl_link_add(sk, link, NLM_F_CREATE);
	if (ret < 0) {
		nl_perror(ret, "Unable to add link");
		goto _error;
	}

	ret = 0;
_error:
	rtnl_link_put(link);

	return ret;
}

int delete_hsr_interface_by_name(const char *if_name)
{
	struct nl_cache *link_cache = NULL;
	struct rtnl_link *hsr_link = NULL;
	struct nl_sock *sk = NULL;
	int ret;

	sk = nl_socket_alloc();
	ret = nl_connect(sk, NETLINK_ROUTE);
	if (ret < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	if (!hsr_link) {
		ret = -NLE_OBJ_NOTFOUND;
		nl_perror(ret, "Deleting failed");
		goto _error;
	}

	ret = rtnl_link_delete(sk, hsr_link);
	if (ret < 0)
		goto _error;

	ret = 0;
_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;
}

int delete_hsr_interface(struct nl_sock *sk, struct rtnl_link *hsr_link)
{
	int ret;

	ret = rtnl_link_delete(sk, hsr_link);
	if (ret < 0) {
		if (ret == -NLE_OBJ_NOTFOUND)
			DLOG_ERR("No matching link exists for delete");
		else
			goto _error;
	}

	ret = 0;
_error:
	return ret;
}

int create_hif_from_waitlist(char *if_name, char *slave_1_name,
			     char *slave_2_name)
{
	struct rtnl_link *slave_1_link, *slave_2_link;
	struct rtnl_link *hsr_link = NULL;
	struct nl_cache *link_cache;
	struct nl_sock *sk;
	int ret;

	sk = nl_socket_alloc();
	ret = nl_connect(sk, NETLINK_ROUTE);
	if (ret < 0) {
		nl_perror(ret, "Unable to connect socket");
		goto _error;
	}

	ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	slave_1_link = rtnl_link_get_by_name(link_cache, slave_1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave_2_name);
	hsr_link = rtnl_link_get_by_name(link_cache, if_name);

	if (!hsr_link) {
		ret = create_hsr_interface(sk, if_name, slave_1_link,
					   slave_2_link, 1);
		if (ret < 0)
			goto _error;
	} else {
		ret = recreate_hsr_interface(sk, hsr_link, slave_2_link,
					     slave_2_link, 1);
		if (ret < 0)
			goto _error;
	}

_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);
	nl_close(sk);
	return ret;
}

int recreate_hsr_interface(struct nl_sock *sk, struct rtnl_link *old_hsr_link,
			   struct rtnl_link *slave_1, struct rtnl_link *slave_2,
			   int version)
{
	int ret;

	ret = delete_hsr_interface(sk, old_hsr_link);
	if (ret < 0)
		goto _error;

	ret = create_hsr_interface(sk, rtnl_link_get_name(old_hsr_link), slave_1,
				   slave_2, version);
	if (ret < 0)
		goto _error;

	ret = 0;
_error:

	return ret;
}

int get_master(struct nl_sock *sk, int slave, struct rtnl_link **master)
{
	struct nl_cache *link_cache;
	struct nl_object *hsr_obj;
	int ret;

	ret = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	for (NL_CACHE_ELEMENTS(link_cache, hsr_obj)) {
		struct rtnl_link *link = nl_object_priv(hsr_obj);

		if (!rtnl_link_is_hsr(link))
			continue;

		int link_slave1 = rtnl_link_hsr_get_slave1(link);
		int link_slave2 = rtnl_link_hsr_get_slave2(link);

		if (slave == link_slave1 || slave == link_slave2) {
			*master = link;
			return 1;
		}
	}

	ret = 0;
_error:
	nl_cache_put(link_cache);

	return ret;
}

//This is a check for the presence of slave links.
//The configuration will not be applied in any case.
//Cases:
// 1. Both links not exists.
// 2.1 The first link does not exists, the second one exists and it have a master.
// 2.2 The first link does not exists, the second one exists and it don't have a master.
// 3.1 The second link does not exists, the first one exists and it have a master.
// 3.2 The second link does not exists, the first one exists and it don't have a master.
static void cnp_some_link_not_exists(struct rtnl_link *slave_1_link,
				     struct rtnl_link *slave_2_link,
				     struct rtnl_link *slave1_master,
				     struct rtnl_link *slave2_master,
				     struct hsr_interface *intrf)
{
	if (!slave_1_link && !slave_2_link) {
		set_status_flags(intrf, SLAVE1_NOT_EXISTS | SLAVE2_NOT_EXISTS);
		DLOG_NOTICE("Both link not exists");

	} else if (!slave_1_link) {
		set_status_flags(intrf, SLAVE1_NOT_EXISTS);
		if (slave2_master)
			set_status_flags(intrf, SLAVE2_BUSY);

		DLOG_NOTICE("Slave1 not exists, slave2 exists");

	} else {
		set_status_flags(intrf, SLAVE2_NOT_EXISTS);
		if (slave1_master)
			set_status_flags(intrf, SLAVE1_BUSY);

		DLOG_NOTICE("Slave2 not exists, slave1 exists");
	}
}

//We remove the interface from the host when:
// 1. The slaves from the notification correspond to the slaves from the interface
//in the host.
// 2.1 One of the slaves from the notification correspond to one of the slave from the
//interface in the host. And the other slave does not have a master.
// 2.2 One of the slaves from the notification correspond to one of the slave from the
//interface in the host.
// 3. Both slaves does not have a master.
static void cnp_hsr_link_exists(struct hsr_module *app,
				struct rtnl_link *slave_1_link,
				struct rtnl_link *slave_2_link,
				struct rtnl_link *slave1_master,
				struct rtnl_link *slave2_master,
				struct rtnl_link *hsr_link,
				struct hsr_interface *intrf)
{
	int hlink_sl1 = rtnl_link_hsr_get_slave1(hsr_link);
	int hlink_sl2 = rtnl_link_hsr_get_slave2(hsr_link);
	int slave1_ifindex = rtnl_link_get_ifindex(slave_1_link);
	int slave2_ifindex = rtnl_link_get_ifindex(slave_2_link);
	int ret;

	if ((slave1_ifindex == hlink_sl1 &&
	     slave2_ifindex == hlink_sl2) ||
	    (slave2_ifindex == hlink_sl1 &&
	     slave1_ifindex == hlink_sl2)) {
		ret = delete_hsr_interface(app->route_manager->sk, hsr_link);
		if (ret < 0)
			DLOG_ERR("Filed to delete hsr interface");
		DLOG_NOTICE("Hsr link exists, slave1 and slave2 matches");
	} else {
		if (slave1_ifindex == hlink_sl1 || slave1_ifindex == hlink_sl2) {
			if (!slave2_master) {
				ret = delete_hsr_interface(app->route_manager->sk,
							   hsr_link);
				if (ret < 0)
					DLOG_ERR("Filed to delete hsr interface");
				DLOG_NOTICE("Hsr link exists,"
				" slave1 matches, slave2 free");
			} else {
				set_status_flags(intrf, SLAVE2_BUSY);
				DLOG_NOTICE("Hsr link exists,"
				" slave1 matches, slave2 exists and busy");
			}

		} else if ((slave2_ifindex == hlink_sl1) ||
			   (slave2_ifindex == hlink_sl2)) {
			if (!slave1_master) {
				ret = delete_hsr_interface(app->route_manager->sk,
							   hsr_link);
				if (ret < 0)
					DLOG_ERR("Filed to delete hsr interface");
				DLOG_NOTICE("Hsr link exists,"
				" slave2 matches, slave1 exists");
			} else {
				set_status_flags(intrf, SLAVE1_BUSY);
				DLOG_NOTICE("Hsr link exists,"
				" slave2 matches, slave1 exists and busy");
			}

		} else {
			if (!slave1_master && !slave2_master) {
				ret = delete_hsr_interface(app->route_manager->sk,
							   hsr_link);
				if (ret < 0)
					DLOG_ERR("Filed to delete hsr interface");
				DLOG_NOTICE("Hsr link exists, slave1 and slave2 exists");
			} else if (slave1_master && slave2_master) {
				set_status_flags(intrf, SLAVE1_BUSY | SLAVE2_BUSY);
				DLOG_NOTICE("Hsr link exists,"
				" slave1 and slave2 exists and busy");
			}
		}
	}
}

//We create hsr interface when both slave links from notification does not have master
static void cnp_hsr_link_not_exixts(struct hsr_module *app,
				    const char *if_name,
				    struct rtnl_link *slave_1_link,
				    struct rtnl_link *slave_2_link,
				    struct rtnl_link *slave1_master,
				    struct rtnl_link *slave2_master,
				    struct hsr_interface *intrf)
{
	int ret;

	if (!slave1_master && !slave2_master) {
		//очень сомнительный блок, не помню зачем такую проверку вставил
		if (avl_is_empty(&app->interfaces)) {
			ret = create_hsr_interface(app->route_manager->sk,
						   (char *)if_name,
						   slave_1_link,
						   slave_2_link, 1);
			if (ret < 0)
				DLOG_ERR("Failed to create hsr interface '%s'", if_name);
			intrf->applied = true;
		} else {
			intrf->applied = false;
		}
		DLOG_NOTICE("Hsr link not exists, slave1 and slave2 exists and free\n");

	} else if (slave1_master && slave2_master) {
		set_status_flags(intrf, SLAVE1_BUSY | SLAVE2_BUSY);
		DLOG_NOTICE("Hsr link not exists, slave1 and slave2 exists and busy\n");
	} else if (slave1_master && (!slave2_master)) {
		set_status_flags(intrf, SLAVE1_BUSY);
		DLOG_NOTICE("Hsr link not exists, slave1 busy, slave2 exists and free\n");
	} else if (!slave1_master && slave2_master) {
		set_status_flags(intrf, SLAVE2_BUSY);
		DLOG_NOTICE("Hsr link not exists, slave2 busy, slave1 exists and free\n");
	}
}

//Each notification from confdb may contain a configuration that we will not be able to
//apply immediately.This case corresponds to the situation when some child link or both
//does not exist.
//If both child links exist, further actions depend on the presence of the hsr interface
//with the name, as in the confdb notification.
//
//In general, if the hsr interface with the same name is present, then the old link
//should first be deleted, and then created again with the desired configuration.
//Otherwise, we create the hsr interface immediately.
static void status_determination(struct hsr_module *app,
				 const char *if_name,
				 struct rtnl_link *slave_1_link,
				 struct rtnl_link *slave_2_link,
				 struct rtnl_link *hsr_link,
				 struct hsr_interface *intrf)
{
	struct rtnl_link *slave1_master = NULL;
	struct rtnl_link *slave2_master = NULL;
	int slave1_ifindex = rtnl_link_get_ifindex(slave_1_link);
	int slave2_ifindex = rtnl_link_get_ifindex(slave_2_link);

	get_master(app->route_manager->sk, slave1_ifindex, &slave1_master);
	get_master(app->route_manager->sk, slave2_ifindex, &slave2_master);

	if (!slave_1_link || !slave_2_link) {
		cnp_some_link_not_exists(slave_1_link,
					 slave_2_link,
					 slave1_master,
					 slave2_master,
					 intrf);

	} else {
		if (hsr_link) {
			cnp_hsr_link_exists(app,
					    slave_1_link,
					    slave_2_link,
					    slave1_master,
					    slave2_master,
					    hsr_link,
					    intrf);
		} else {
			cnp_hsr_link_not_exixts(app,
						if_name,
						slave_1_link,
						slave_2_link,
						slave1_master,
						slave2_master,
						intrf);
		}
	}
}

int confdb_notification_processing(struct hsr_module *app,
				   const char *if_name,
				   const char *slave1_name,
				   const char *slave2_name)
{
	struct rtnl_link *slave_1_link, *slave_2_link;
	struct hsr_interface *intrf = NULL;
	struct rtnl_link *hsr_link = NULL;
	struct nl_cache *link_cache;
	int ret;

	ret = rtnl_link_alloc_cache(app->route_manager->sk, AF_UNSPEC, &link_cache);
	if (ret < 0) {
		nl_perror(ret, "Unable to allocate cache");
		goto _error;
	}

	slave_1_link = rtnl_link_get_by_name(link_cache, slave1_name);
	slave_2_link = rtnl_link_get_by_name(link_cache, slave2_name);
	hsr_link = rtnl_link_get_by_name(link_cache, if_name);
	printf("439_confdb_notification_processing\n");
	ret = add_interface_to_dict(app, &intrf, if_name, slave1_name, slave2_name);
	if (ret < 0)
		goto _error;
printf("443_confdb_notification_processing\n");
	status_determination(app, if_name, slave_1_link, slave_2_link, hsr_link, intrf);
printf("445_confdb_notification_processing\n");
	print_interfaces_dict(app);
	printf("447_confdb_notification_processing\n");

_error:
	if (hsr_link)
		rtnl_link_put(hsr_link);
	nl_cache_put(link_cache);

	return ret;
}
