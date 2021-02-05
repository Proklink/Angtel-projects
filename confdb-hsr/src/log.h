
#ifndef _CFD_LOG_H_
#define _CFD_LOG_H_

#include <stdarg.h>
#include <liblogging/stdlog.h>
#include <jansson.h>
#include <netlink/object.h>

extern stdlog_channel_t debug_logger;
extern unsigned int debug_level;

int dvlog(const int severity, const char *fmt, va_list ap);
int dlog(const int severity, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
int dlog_jerr(const json_error_t *err, const int severity, const char *fmt,
	      ...) __attribute__((format(printf, 3, 4)));
void dlog_json_dump(int sev, json_t *jv, const char *ttl);
void dlog_cache_change(const char *cache_name, int log_level,
		       struct nl_object *obj, uint64_t attr_diff, int nl_act,
		       enum nl_dump_type type);

#ifndef APP_LOG_NAME
#define DLOG(severity, fmt, ...) \
	dlog(severity, fmt, ##__VA_ARGS__)
#define DLOG_JERR(err, fmt, ...) \
	dlog_jerr(err, STDLOG_ERR, fmt, ##__VA_ARGS__)
#else
#define DLOG(severity, fmt, ...) \
	dlog(severity, "%s: " fmt, APP_LOG_NAME, ##__VA_ARGS__)
#define DLOG_JERR(err, fmt, ...) \
	dlog_jerr(err, STDLOG_ERR, "%s: " fmt, APP_LOG_NAME, ##__VA_ARGS__)
#endif

#define DLOG_FUNC(severity, fmt, ...) \
	DLOG(severity, "%s: " fmt, __func__, ##__VA_ARGS__)
#define DLOG_JERR_FUNC(err, fmt, ...) \
	DLOG_JERR(err, "%s: " fmt, __func__, ##__VA_ARGS__)

#define DLOG_EMERG(...) DLOG(STDLOG_EMERG, __VA_ARGS__)
#define DLOG_ALERT(...) DLOG(STDLOG_ALERT, __VA_ARGS__)
#define DLOG_CRIT(...) DLOG(STDLOG_CRIT, __VA_ARGS__)
#define DLOG_ERR(...) DLOG(STDLOG_ERR, __VA_ARGS__)
#define DLOG_WARNING(...) DLOG(STDLOG_WARNING, __VA_ARGS__)
#define DLOG_NOTICE(...) DLOG(STDLOG_NOTICE, __VA_ARGS__)
#define DLOG_INFO(...) DLOG(STDLOG_INFO, __VA_ARGS__)
#define DLOG_DEBUG(...) DLOG(STDLOG_DEBUG, __VA_ARGS__)

#define DLOG_EMERG_FUNC(...) DLOG_FUNC(STDLOG_EMERG, __VA_ARGS__)
#define DLOG_ALERT_FUNC(...) DLOG_FUNC(STDLOG_ALERT, __VA_ARGS__)
#define DLOG_CRIT_FUNC(...) DLOG_FUNC(STDLOG_CRIT, __VA_ARGS__)
#define DLOG_ERR_FUNC(...) DLOG_FUNC(STDLOG_ERR, __VA_ARGS__)
#define DLOG_WARNING_FUNC(...) DLOG_FUNC(STDLOG_WARNING, __VA_ARGS__)
#define DLOG_NOTICE_FUNC(...) DLOG_FUNC(STDLOG_NOTICE, __VA_ARGS__)
#define DLOG_INFO_FUNC(...) DLOG_FUNC(STDLOG_INFO, __VA_ARGS__)
#define DLOG_DEBUG_FUNC(...) DLOG_FUNC(STDLOG_DEBUG, __VA_ARGS__)

#endif
