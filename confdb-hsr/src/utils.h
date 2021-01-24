#ifndef _CONFIGD_UTILS_H
#define _CONFIGD_UTILS_H

#include <unistd.h>
#include <stdarg.h>
#include <event2/event.h>
#include <jansson.h>

/* Flag manipulation macros. */
#define CHECK_FLAG(V,F)      ((V) & (F))
#define SET_FLAG(V,F)        (V) |= (F)
#define UNSET_FLAG(V,F)      (V) &= ~(F)
#define RESET_FLAG(V)        (V) = 0
#define COND_FLAG(V, F, C)   ((C) ? (SET_FLAG(V, F)) : (UNSET_FLAG(V, F)))

/* Convertions enum-value <-> string */
typedef struct val_str {
	const char *str;
	int val;
} val_str_t;

const char * val2str(int val, const val_str_t *table, const char *def_str);

#define JTSX_MULTITARGET 0x1
#define JTSX_IGNORE_MISSING_DATA 0x2

int jtsx_add_op(json_t *jtsx, const char *fmt, ...);
int jtsx_add_remove_op(json_t *jtsx, int flags, const char *fmt, ...)
	__attribute__((format(printf, 3, 4)));
int jtsx_add_create_op(json_t *jtsx, int flags, json_t *jval, const char *fmt,
		       ...) __attribute__((format(printf, 4, 5)));
int jtsx_add_merge_op(json_t *jtsx, int flags, json_t *jval, const char *fmt,
		      ...) __attribute__((format(printf, 4, 5)));
int jtsx_add_replace_op(json_t *jtsx, int flags, json_t *jval, const char *fmt,
			...) __attribute__((format(printf, 4, 5)));

#endif
