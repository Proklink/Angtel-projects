#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <netlink/cache.h>

#include "utils.h"

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

static int dump_nl_obj(struct nl_object *obj, enum nl_dump_type type,
		       char ***lines, int *num)
{
	int ret;
	char *buf = NULL;
	size_t size;
	FILE *f;
	struct nl_dump_params params = {0};

	f = open_memstream(&buf, &size);
	if (!f) {
		DLOG_ERR_FUNC("failed to open_memstream: %s", strerror(errno));
		return -1;
	}

	params.dp_type = type,
	params.dp_fd = f;

	nl_object_dump(obj, &params);

	ret = fclose(f);
	if (ret != 0) {
		DLOG_ERR_FUNC("failed to fclose: %s", strerror(errno));
		return -1;
	}

	ret = str_split(buf, "\n", lines, num);
	if (ret < 0) {
		DLOG_ERR_FUNC("str_split failed");
		return -1;
	}

	free(buf);

	assert(*num > 0);

	if (*num > 1 && strlen((*lines)[*num-1]) == 0) {
		/* nl_object_dump adds '\n' at the end of output. */
		free((*lines)[*num-1]);
		(*num)--;
	}

	return 0;
}


static const char *act2str(int val)
{
	const char *tbl[] = {
		[NL_ACT_UNSPEC] = "unspec",
		[NL_ACT_NEW] = "new",
		[NL_ACT_DEL] = "del",
		[NL_ACT_GET] = "get",
		[NL_ACT_SET] = "set",
		[NL_ACT_CHANGE] = "change",
	};

	return tbl[val];
}

void dlog_cache_change(const char *cache_name, int log_level,
		       struct nl_object *obj, uint64_t attr_diff, int nl_act,
		       enum nl_dump_type type)
{
	int ret;
	char **lines;
	int num;
	char attr_buf[256];
	int indent;
	const char *act = act2str(nl_act);

	ret = dump_nl_obj(obj, type, &lines, &num);
	if (ret < 0) {
		DLOG_ERR("%s: %s: failed to dump nl object", cache_name, act);
		return;
	}

	indent = strlen(cache_name) + 2;

	if (type == NL_DUMP_LINE) {
		DLOG(log_level, "%s: %s: %s", cache_name, act, lines[0]);
	} else {
		DLOG(log_level, "%s: %s", cache_name, act);
		for (int i = 0; i < num; i++)
			DLOG(log_level, "%*s%s", indent, "", lines[i]);
	}

	str_free_split(lines, num);

	if (nl_act == NL_ACT_CHANGE) {
		nl_object_attrs2str(obj, (uint32_t)attr_diff, attr_buf,
				    sizeof(attr_buf));
		DLOG(log_level, "%*sdiff: 0x%" PRIx64 " (%s)", indent, "",
		     attr_diff, attr_buf);
	}
}
