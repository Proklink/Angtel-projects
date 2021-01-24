
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <assert.h>

#include "confdb.h"
#include "log.h"
#include "utils.h"

const char *val2str(int val, const val_str_t *table, const char *def_str)
{
	int i = 0;
	while (1) {
		if (table[i].str == NULL)
			break;
		if (table[i].val == val) {
			return table[i].str;
		}
		i++;
	}
	return def_str;
}

int jtsx_add_op(json_t *jtsx, const char *fmt, ...)
{
	va_list ap;
	json_error_t err;
	int ret = 0;
	json_t *jop;

	va_start(ap, fmt);
	jop = json_vpack_ex(&err, 0, fmt, ap);
	if (!jop) {
		DLOG_JERR(&err, "Failed to pack json operation");
		ret = -1;
		goto on_exit;
	}

	ret = json_array_append_new(jtsx, jop);
	if (ret == -1) {
		DLOG_ERR("Failed to append operation to transaction");
		goto on_exit;
	}

on_exit:
	va_end(ap);
	return ret;
}

static int jtsx_add_op_tmpl(json_t *jtsx, int flags, json_t *jval,
			    const char *fmt, va_list ap, enum cdb_op_type op)
{
	val_str_t tbl[] = {
		{ "create", CDB_OP_CREATE },
		{ "create-child", CDB_OP_CREATE_CHILD },
		{ "delete", CDB_OP_DELETE },
		{ "replace", CDB_OP_REPLACE },
		{ "merge", CDB_OP_MERGE },
		{ "remove", CDB_OP_REMOVE },
		{ "move", CDB_OP_MOVE },
		{ NULL },
	};
	int ret = 0;
	char *target = NULL;
	json_t *jmulti = NULL;
	json_t *jignore = NULL;
	json_t *jvalue = NULL;
	const char *op_str;

	if (op != CDB_OP_DELETE && op != CDB_OP_REMOVE && !jval) {
		DLOG_ERR_FUNC("Invalid input 'jval' argument");
		return -1;
	}

	op_str = val2str(op, tbl, NULL);
	assert(op_str);

	ret = vasprintf(&target, fmt, ap);
	if (ret == -1) {
		DLOG_ERR_FUNC("vasprintf failed");
		return -1;
	}

	if (CHECK_FLAG(flags, JTSX_MULTITARGET))
		jmulti = json_true();

	if (CHECK_FLAG(flags, JTSX_IGNORE_MISSING_DATA))
		jignore = json_true();

	if (jval)
		jvalue = json_pack("{so}", basename(target), jval);

	ret = jtsx_add_op(jtsx, "{ss ss so* so* so*}",
			  "operation", op_str,
			  "target", target,
			  "multitarget", jmulti,
			  "ignore-missing-data", jignore,
			  "value", jvalue);

	free(target);
	return ret;
}

int jtsx_add_remove_op(json_t *jtsx, int flags, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = jtsx_add_op_tmpl(jtsx, flags, NULL, fmt, ap, CDB_OP_REMOVE);
	va_end(ap);
	return ret;
}

int jtsx_add_create_op(json_t *jtsx, int flags, json_t *jval, const char *fmt,
		       ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = jtsx_add_op_tmpl(jtsx, flags, jval, fmt, ap, CDB_OP_CREATE);
	va_end(ap);
	return ret;
}

int jtsx_add_merge_op(json_t *jtsx, int flags, json_t *jval, const char *fmt,
		      ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = jtsx_add_op_tmpl(jtsx, flags, jval, fmt, ap, CDB_OP_MERGE);
	va_end(ap);
	return ret;
}

int jtsx_add_replace_op(json_t *jtsx, int flags, json_t *jval, const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = jtsx_add_op_tmpl(jtsx, flags, jval, fmt, ap, CDB_OP_REPLACE);
	va_end(ap);
	return ret;
}
