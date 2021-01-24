
#include "log.h"

stdlog_channel_t debug_logger;
unsigned int debug_level = 6;

static int dvlog_channel(stdlog_channel_t channel, const int severity, const char *fmt, va_list ap)
{
	int ret = 0;

	if (severity < debug_level) {
		ret = stdlog_vlog(channel, severity, fmt, ap);
	}
	return ret;
}

int dvlog(const int severity, const char *fmt, va_list ap)
{
	return dvlog_channel(debug_logger, severity, fmt, ap);
}

int dlog(const int severity, const char *fmt, ...)
{
	va_list ap;
	int ret = 0;

	va_start(ap, fmt);
	ret = dvlog_channel(debug_logger, severity, fmt, ap);
	va_end(ap);
	return ret;
}

int dlog_jerr(const json_error_t *err, const int severity, const char *fmt, ...)
{
	char *str;
	int ret;

	va_list ap;
	va_start(ap, fmt);
	ret = vasprintf(&str, fmt, ap);
	va_end(ap);
	if (ret < 0)
		return ret;

	ret = dlog(severity, "%s: %s, %s, line %d, column %d", str, err->text,
		   err->source, err->line, err->column);

	free(str);
	return ret;
}

void dlog_json_dump(int sev, json_t *jv, const char *ttl)
{
	char *dump = json_dumps(jv, JSON_INDENT(2) | JSON_ENCODE_ANY);
	if (!dump) {
		DLOG_ERR("Dump '%s' failed", ttl);
		return;
	}

	dlog(sev, "%s:\n%s", ttl, dump);
	free(dump);
}
