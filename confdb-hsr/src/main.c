#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <stdatomic.h>
#include "apply_conf.h"

#include <jansson.h>
#include <confdb/confdb.h>
#include <confdb/log.h>

#include "log.h"

#include "utils.h"

static volatile atomic_int keep_running;

#define MODULE_NAME "hsr"
#define XPATH_ITF "/ietf-interfaces:interfaces/interface"

struct hsr_module {
	struct confdb *cdb;
	char *slave_a;
	char *slave_b;
	int num_dev_in_ring;
	
};

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

/*
 * Функция, переводящая время в ISO-формат.
 *
 * @timestamp - Временная метка (результат вызова функции time()).
 * @iso_time - Буфер для хранения времени в ISO-формате.
 * @iso_time - Размер буфера @iso_time.
 */
/*static int timestamp_to_iso8601(time_t timestamp, char *iso_time, size_t length)
{
	struct tm *local_timestamp_bd;

	char local_timestamp[64];
	char timezone[8];
	char timezone_hours[8] = { 0 };
	char timezone_minutes[8] = { 0 };

	size_t strftime_rv;
	int snprintf_rv;

	local_timestamp_bd = localtime(&timestamp);
	if (local_timestamp_bd == NULL) {
		DLOG_FUNC(STDLOG_ERR, "Unable to create local timestamp.");
		return -1;
	}

	strftime_rv = strftime(local_timestamp, sizeof(local_timestamp),
			       "%Y-%m-%dT%H:%M:%S", local_timestamp_bd);
	if (strftime_rv == 0) {
		DLOG_FUNC(STDLOG_ERR, "Unable to format local timestamp.");
		return -1;
	}

	strftime_rv = strftime(timezone, sizeof(timezone),
			       "%z", local_timestamp_bd);
	if (strftime_rv == 0) {
		DLOG_FUNC(STDLOG_ERR, "Unable to format local timezone.");
		return -1;
	}

	memcpy(timezone_hours, timezone, strlen(timezone) - 2);
	memcpy(timezone_minutes, timezone + strlen(timezone) - 2, 2);

	snprintf_rv = snprintf(iso_time, length,
			       "%s%s:%s", local_timestamp,
			       timezone_hours, timezone_minutes);
	if (snprintf_rv == -1) {
		DLOG_FUNC(STDLOG_ERR, "Unable to print local timestamp.");
		return -1;
	}

	return 0;
}*/

/*
 * Функция, создающая статусные данные.
 */
/*static json_t *create_cdb_data(void)
{
	json_t *json_data;
	time_t timestamp;
	char timestamp_string[64];
	int ret;

	timestamp = time(NULL);
	ret = timestamp_to_iso8601(timestamp,
				   timestamp_string,
				   sizeof(timestamp_string));
	if (ret == -1)
		return NULL;

	json_data = json_string(timestamp_string);
	if (json_data == NULL)
		return NULL;

	return json_data;
}*/

/*static int app_edit_config(struct hsr_module *app, const char *iface_name,
			   const char *value)
{
	int ret;
	json_t *jv;
	char *slave_a = "eth0";
	char *slave_b = "enp2s0";

	jv = json_pack("{s:s, s:s}", "slave-a", &slave_a,
				     			 "slave-b", &slave_b);
	if (!jv) {
		DLOG_ERR("Failed to pack jv");
		return -1;
	}

	ret = cdb_edit_config(app->cdb, CDB_OP_MERGE, jv,
			      "/ietf-interfaces:interfaces/interface[name='%s']/angtel-hsr:hsr {slave_a, slave_b}", 
				  																				iface_name);
	if (ret < 0) {
		DLOG_ERR("Failed to cdb_edit_status");
		return -1;
	}

	return 0;
}*/

/*
 * Функция, демонстрирующая запись данных (в т.ч. статусных) в БД.
 *
 * @mod - Указатель на модуль configd.
 * @interface_name - Имя сетевого интерфейса для которого записывается статус.
 */
/*static int write_status(struct confdb *cdb, const char *interface_name)
{
	json_t *jtsx;
	json_t *json_data;
	int ret;

	
	 // Транзакция к CDB которая содержит в себе различные операции:
	 // delete, remove, create, merge, replace и т.д.
	 // К данной транзакции будут добавляться желаемые операции с данными.
	 //
	jtsx = json_array();
	if (jtsx == NULL) {
		DLOG_FUNC(STDLOG_ERR, "Unable to create CDB transaction.");
		return -1;
	}

	json_data = create_cdb_data();
	if (json_data == NULL)
		return -1;

	// Добавление операции к транзакции. 
	ret = jtsx_add_merge_op(jtsx, 0, json_data,
				"/ietf-interfaces:interfaces"
				"/interface {\"ietf-interfaces:interface\":[{\"name\":\"hsr1\","
				"\"type\":\"angtel-interfaces:hsr\",\"angtel-hsr:hsr\":{\"slave-a\":\"enp2s0\","
				"\"slave-b\":\"eth0\"}}]}",
				interface_name);
	if (ret == -1) {
		DLOG_FUNC(STDLOG_ERR, "Unable to merge CDB object.");
		return -1;
	}

	ret = cdb_edit_status_tsx(cdb, jtsx);
	if (ret == -1) {
		DLOG_FUNC(STDLOG_ERR, "Unable to execute CDB transaction.");
		return -1;
	}

	return 0;
}
*/

static int 	hsr_handler(json_t *cdb_data, json_t *key, json_t *error, void *data)
{
	/* Получение приватных данных модуля hsr программы configd. */
	
	//struct hsr_module *app = data;
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

		ret = change_analysis(interface_name, slave_a, slave_b);
		
	} else {
		/* Данные удалены. */
		printf("Data deleted: %s\n", interface_name);

		delete_hsr_interface(interface_name);
	}

	// /* Запись статуса в БД. */
	// ret = write_status(app->cdb, interface_name);
	// if (ret == -1)
	// 	DLOG_FUNC(STDLOG_ERR,
	// 		  "Unable to write status for interface %s.",
	// 		  interface_name);


	return 0;
}

/* Подписка отслеживание изменения данных. */
static struct cdb_data_notifier notifiers[] = {
	{
		/* Путь XPath в схеме по которому расположены отслеживаемые данные. */
		.schema_xpath = "/ietf-interfaces:interfaces/interface"
				"/angtel-hsr:hsr",
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

	cdb_remove_extension(app->cdb, "hsr-app");
	cdb_wait_for_responses(app->cdb);
	cdb_destroy(app->cdb);
	free(app->slave_a);
	free(app->slave_b);
	free(app);
}

static struct hsr_module *app_create(void)
{
	int ret;
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
	int fd;

	opt_ini_or_die(argc, argv);

	keep_running = 1;
	signal(SIGTERM, stop_cleds);
	signal(SIGINT, stop_cleds);

	app = app_create();
	if (!app)
		goto on_error;

	fd = cdb_get_fd(app->cdb);
	if (fd < 0) {
		DLOG_ERR("Failed to get file descriptor for confdb\n");
			return -1;
	}

	
	while (keep_running) {
	
		int max_fd;
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		max_fd = fd;

		ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			DLOG_ERR("Select failed: %s", strerror(errno));
			goto on_error;
		}

		ret = cdb_dispatch(app->cdb, &fds);
		if (ret == -CDB_ERR_CONNRESET) {
			DLOG_INFO("CDB server closed connection");
			break;
		} else if (ret < 0) {
			DLOG_ERR("cdb_dispatch failed: %s", cdb_strerror(ret));
			goto on_error;
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
