#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <jansson.h>


#include <linux/if.h>

#include <netlink/route/link.h>

#include "log.h"
#include "apply_conf.h"
#include "utils.h"
//#include "status.h"


static volatile atomic_int keep_running;



static void hnode_cache_change_cb(struct nl_cache *cache,
				       struct nl_object *o_obj,
				       struct nl_object *n_obj,
				       uint64_t attr_diff, int nl_act, void *data)
{
	dlog_cache_change("hsr_node", STDLOG_INFO, n_obj ?: o_obj, attr_diff,
			  nl_act, NL_DUMP_LINE);

	printf("\nhnode_cache_change_cb\n");

	struct hsr_module *app = data;

	if (nl_act == NL_ACT_NEW) {
		add_node_to_list(app, n_obj);
	} else if (nl_act == NL_ACT_DEL)
		delete_node_from_list(app, n_obj);

	
}

static bool cdb_log_enable;
static int cdb_logger(const int severity, const char *fmt, va_list ap)
{
	int s;

	if (!cdb_log_enable)
		return 0;

	switch (severity) {
	case CDBLOG_EMERG: s = STDLOG_EMERG; break;
	case CDBLOG_ALERT: s = STDLOG_ALERT; break;
	case CDBLOG_CRIT: s = STDLOG_CRIT; break;
	case CDBLOG_ERR: s = STDLOG_ERR; break;
	case CDBLOG_WARNING: s = STDLOG_WARNING; break;
	case CDBLOG_NOTICE: s = STDLOG_NOTICE; break;
	case CDBLOG_INFO: s = STDLOG_INFO; break;
	default: s = STDLOG_DEBUG; break;
	}
	dvlog(s, fmt, ap);
	return 0;
}

static void opt_ini_or_die(int argc, char *argv[])
{
	int opt;
	char *log_spec = "stdout:";

	while ((opt = getopt(argc, argv, "hvL:d:C")) != -1) {
		switch (opt) {
		case 'L':
			log_spec = optarg;
			if (strcmp(log_spec, "stdout:") != 0 &&	strncmp(log_spec, "file:", 5) != 0 &&
			    strncmp(log_spec, "uxsock:", 7) != 0 && strncmp(log_spec, "syslog:", 7) != 0) {
				fprintf(stderr, "Invalid log spec\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'd':
			debug_level = atoi(optarg);
			if (debug_level > 8) {
				fprintf(stderr, "Invalid debug level\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'C':
			cdb_log_enable = true;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	debug_logger = stdlog_open("hsr-app", 0, STDLOG_USER, log_spec);
	if (debug_logger == NULL) {
		fprintf(stderr, "Failed to open debug logger\n");
		exit(EXIT_FAILURE);
	}

	cdb_set_logger(cdb_logger, debug_level);
}

static void opt_free(void)
{
	stdlog_close(debug_logger);
}


static int hsr_handler(json_t *cdb_data, json_t *key, json_t *error, void *data)
{
	/* Получение приватных данных модуля hsr программы configd. */
	
	struct hsr_module *app = data;
	const char *interface_name;
	
	int ret;

	printf("\nstatus: %s\n", json_dumps(cdb_data, JSON_ENCODE_ANY));

	interface_name = json_string_value(key);
	if (interface_name == NULL) {
		DLOG_FUNC(STDLOG_ERR, "Unable to get interface name.");
		return -1;
	}

	if (cdb_data != NULL) {
		const char *slave_a = NULL, *slave_b = NULL;
		json_error_t json_error;
	

		/* Данные добавлены или изменены. */
		printf("Data added or changed: %s\n", interface_name);

		/* Извлечение добавленных или измененных данных. */
		ret = json_unpack_ex(cdb_data, &json_error, 0,
				     "{s:s, s:s}",
				     "slave-a", &slave_a,
				     "slave-b", &slave_b);
		if (ret == -1) {
			DLOG_FUNC(STDLOG_ERR,
				  "Error: %s. Source: %s. Line: %d. Column: %d.",
				  json_error.text, json_error.source,
				  json_error.line, json_error.column);
			return -1;
		}

		/* Здесь можно добавить какие-то полезные действия. */
		printf("slave-a: %s, slave-b: %s\n\n", slave_a, slave_b);

		ret = change_analysis(app, interface_name, slave_a, slave_b);
		if (ret < 0) {
			return ret;
		}

	} else {
		/* Данные удалены. */
		printf("Data deleted: %s\n", interface_name);

		delete_hsr_interface(interface_name);

		struct nl_cache *mngr_hnode_cache = __nl_cache_mngt_require("hsr_node");

		nl_cache_clear(mngr_hnode_cache);
	}

	return 0;
}


/* Подписка отслеживание изменения данных. */
static struct cdb_data_notifier notifiers[] = {
	{
		/* Путь XPath в схеме по которому расположены отслеживаемые данные. */
		.schema_xpath = XPATH_ITF "/angtel-hsr:hsr",
		/* Обработчик, вызываемый при обнаружении изменения данных. */
		.handler = hsr_handler,
		/* Флаг, обозначающий, что изменение статусных данных нужно игнорировать. */
		.ignore_status = true
	},
	/* Признак завершения массива notifiers. */
	{
		.schema_xpath = NULL
	}
};


static void app_destroy(struct hsr_module *app)
{
	if (!app)
		return;
		
	if (app->nl_mngr)
		nl_cache_mngr_free(app->nl_mngr);

	nl_socket_free(app->sk);

	cdb_remove_extension(app->cdb, "hsr-app");
	cdb_wait_for_responses(app->cdb);
	cdb_destroy(app->cdb);
	free(app);
}

static struct hsr_module *app_create(void)
{
	int ret;
	struct nl_cache *hnode_cache = NULL;
	struct hsr_module *app = NULL;

	app = calloc(1, sizeof(*app));
	if (!app)
		return NULL;

	app->cdb = cdb_create();
	if (!app->cdb) {
		DLOG_ERR("Failed to create CDB client");
		goto on_error;
	}

	ret = cdb_add_extension(app->cdb, "hsr-app", NULL, notifiers, app);
	if (ret < 0) {
		DLOG_ERR("Failed to add extension on CDB client");
		goto on_error;
	}

	app->sk = nl_socket_alloc();
	if (!app->sk) {
		DLOG_ERR("Failed to alloc nl socket");
		goto on_error;
	}

	ret = nl_cache_mngr_alloc(app->sk, NETLINK_GENERIC, NL_AUTO_PROVIDE, &app->nl_mngr);
	if (ret < 0) {
		DLOG_ERR("Failed allocate cache manager: %s", nl_geterror(ret));
		goto on_error;
	}

	nl_cache_mngr_refill_cache_on_adding(app->nl_mngr, false);

	app->cache_mngr_fd = nl_cache_mngr_get_fd(app->nl_mngr);

	// Если кэшей в кэш-менеджер добавлено много и/или происходит много
	// netlink событий - то стандартного размера буфера (32768) может быть
	// недостаточно.
	//				           RX       TX
	ret = nl_socket_set_buffer_size(app->sk, 32*32768, 32*32768);
	if (ret < 0) {
		DLOG_ERR("Failed to set socket buffer size: %s",
			 nl_geterror(ret));
		goto on_error;
	}

	DLOG_INFO("Adding 'hsr_node' cache to cache manager");

	ret = hnode_alloc_cache(app->sk, &hnode_cache);
	if (ret < 0) {
		DLOG_ERR("Failed to allocate 'hsr_node' cache: %s",
		     nl_geterror(ret));
		goto on_error;
	}

	printf("\nAsd1\n");

	ret = nl_cache_mngr_add_cache_v2(app->nl_mngr, hnode_cache, hnode_cache_change_cb, app);
	if (ret < 0) {
		DLOG_ERR("Failed to add 'hsr_node' cache to manager: %s",
			 nl_geterror(ret));
		nl_cache_free(hnode_cache);
		goto on_error;
	
	} 
	
	//ret = fill_all(app);
	

	printf("\nAsd2\n");
	
	// После такого создания кэш-менеджера и добавления в него кэшей
	// доступ к кэшу в любом месте в программе можно делать так:
	// link_cache = nl_cache_mngt_require_safe("route/link")
	// или
	// link_cache = __nl_cache_mngt_require("route/link")
	// в чем отличие - см. libnl

	return app;
on_error:
	app_destroy(app);
	return NULL;
}

void stop_cleds(int sig_num)
{
	keep_running = 0;
}

int main(int argc, char *argv[])
{
	int ret;
	struct hsr_module *app;
	

	opt_ini_or_die(argc, argv);

	keep_running = 1;
	signal(SIGTERM, stop_cleds);
	signal(SIGINT, stop_cleds);

	app = app_create();
	if (!app)
		goto on_error;

	while (keep_running) {
		int fd;
		int max_fd;
		fd_set fds;
		struct timeval to;

		to.tv_sec = 300;
		to.tv_usec = 0;
		FD_ZERO(&fds);
		fd = cdb_prepare_select(app->cdb, &fds, &to);
		max_fd = fd;

		FD_SET(app->cache_mngr_fd, &fds);
		if (app->cache_mngr_fd > max_fd)
			max_fd = app->cache_mngr_fd;

		ret = select(max_fd + 1, &fds, NULL, NULL, &to);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			DLOG_ERR("Select failed: %s", strerror(errno));
			goto on_error;
		}

		if (ret == 0) {
			ret = cdb_check_timeouts(app->cdb);
			if (ret < 0) {
				DLOG_ERR("RPC timeout handling error");
				goto on_error;
			}
			continue;
		}

		ret = cdb_dispatch(app->cdb, &fds);
		if (ret == -CDB_ERR_CONNRESET) {
			DLOG_INFO("CDB server closed connection");
			break;
		} else if (ret < 0) {
			DLOG_ERR("cdb_dispatch failed: %s", cdb_strerror(ret));
			goto on_error;
		}

		if (FD_ISSET(app->cache_mngr_fd, &fds)) {
			printf("\nFD_ISSET\n");
			ret = nl_cache_mngr_data_ready(app->nl_mngr);
			printf("\n%d\n", ret);
			if (ret < 0) {
				DLOG_ERR("Failed to process NL messages: %s",
					 nl_geterror(ret));
				goto on_error;
			}
		}
	}

	app_destroy(app);
	opt_free();
	return 0;
on_error:
	app_destroy(app);
	opt_free();
	exit(EXIT_FAILURE);
}
