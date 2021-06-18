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
#include "status.h"
#include "wait_list.h"


static volatile atomic_int keep_running;


// struct nl_dump_params details_dp = {
// 	.dp_type = NL_DUMP_DETAILS,
// 	.dp_fd = stdout,
// };



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

		FD_SET(app->gen_manager->cache_mngr_fd, &fds);
		FD_SET(app->route_manager->cache_mngr_fd, &fds);
		if (app->gen_manager->cache_mngr_fd > max_fd)
			max_fd = app->gen_manager->cache_mngr_fd;
		if (app->route_manager->cache_mngr_fd > max_fd)
			max_fd = app->route_manager->cache_mngr_fd;

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

		if (FD_ISSET(app->gen_manager->cache_mngr_fd, &fds)) {
			ret = nl_cache_mngr_data_ready(app->gen_manager->nl_mngr);
			if (ret < 0) {
				DLOG_ERR("Failed to process NL messages: %s",
					 nl_geterror(ret));
				goto on_error;
			}
		}

		if (FD_ISSET(app->route_manager->cache_mngr_fd, &fds)) {
			ret = nl_cache_mngr_data_ready(app->route_manager->nl_mngr);
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
