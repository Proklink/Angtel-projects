
#ifndef _CONFDB_CONFDB_H
#define _CONFDB_CONFDB_H

#include <time.h>
#include <jansson.h>
#include <libubox/avl.h>
#include <confdb/ipc.h>

struct confdb;

enum cdb_op_type {
	CDB_OP_CREATE,
	CDB_OP_CREATE_CHILD,
	CDB_OP_INSERT,
	CDB_OP_DELETE,
	CDB_OP_REPLACE,
	CDB_OP_MERGE,
	CDB_OP_REMOVE,
	CDB_OP_MOVE,
};

enum cdb_op_where {
	CDB_BEFORE,
	CDB_AFTER,
	CDB_FIRST,
	CDB_LAST,
};

typedef int (*cdb_rpc_notifier_fn_t) (struct cdb_rpc *rpc, json_t *input,
				      json_t *key, void *data);

struct cdb_rpc_notifier {
	const char *schema_xpath;
	cdb_rpc_notifier_fn_t handler;
	bool async;
};

typedef int (*cdb_data_notifier_fn_t) (json_t *cdb_data, json_t *key, json_t *error, void *data);

struct cdb_data_notifier {
	const char *schema_xpath;
	cdb_data_notifier_fn_t handler;
	bool ignore_status;
	bool show_hidden;
};

int cdb_save(struct confdb *cdb);
int cdb_load(struct confdb *cdb);
int cdb_reset(struct confdb *cdb);

/* For cdb-related applications only. */
int cdb_get_raw(struct confdb *cdb,
		cdb_rpc_rsp_handler_t rsp_handler,
		void *handler_data,
		const char *method,
		const char *filter_str, ...);

int cdb_get(struct confdb *cdb,
	    const struct cdb_data_notifier *data_ntfy,
	    const void *handler_data);

int cdb_edit_tsx(struct confdb *cdb, json_t *params);
int cdb_edit_config_tsx(struct confdb *cdb, json_t *params);
int cdb_edit_status_tsx(struct confdb *cdb, json_t *params);

int cdb_edit(struct confdb *cdb,
	     enum cdb_op_type op,
	     json_t *value,
	     const char *pointer,
	     ...);
int cdb_edit_config(struct confdb *cdb,
		    enum cdb_op_type op,
		    json_t *value,
		    const char *pointer,
		    ...);
int cdb_edit_status(struct confdb *cdb,
		    enum cdb_op_type op,
		    json_t *value,
		    const char *pointer,
		    ...);
int cdb_multitarget_edit(struct confdb *cdb,
			 enum cdb_op_type op,
			 json_t *value,
			 json_t *new_keys,
			 const char *pointer,
			 ...);
int cdb_multitarget_edit_config(struct confdb *cdb,
				enum cdb_op_type op,
				json_t *value,
				json_t *new_keys,
				const char *pointer,
				...);
int cdb_multitarget_edit_status(struct confdb *cdb,
				enum cdb_op_type op,
				json_t *value,
				json_t *new_keys,
				const char *pointer,
				...);

int cdb_move(struct confdb *cdb,
	     const char *pointer,
	     enum cdb_op_where where,
	     const char *point);

int cdb_move_config(struct confdb *cdb,
		    const char *pointer,
		    enum cdb_op_where where,
		    const char *point);
int cdb_move_status(struct confdb *cdb,
		    const char *pointer,
		    enum cdb_op_where where,
		    const char *point);

int cdb_start_transaction(struct confdb *cdb,
			  bool config,
			  bool status,
			  unsigned int timeout_sec);
int cdb_commit(struct confdb *cdb);
int cdb_discard_changes(struct confdb *cdb);

int cdb_subscribe(struct confdb *cdb,
		  const struct cdb_data_notifier *data_ntfy,
		  const void *handler_data);
int cdb_unsubscribe(struct confdb *cdb, const struct cdb_data_notifier *data_ntfy);
int cdb_subscribe_rpc(struct confdb *cdb,
		      const struct cdb_rpc_notifier *rpc_ntfy,
		      const void *handler_data);
int cdb_unsubscribe_rpc(struct confdb *cdb, const struct cdb_rpc_notifier *rpc_ntfy);

int cdb_call_yrpc(struct confdb *cdb,
		  cdb_rpc_rsp_handler_t rsp_handler,
		  void *handler_data,
		  const char *rpc,
		  json_t *yrpc_params);
int cdb_call_action(struct confdb *cdb,
		    cdb_rpc_rsp_handler_t rsp_handler,
		    void *handler_data,
		    const char *action,
		    json_t *action_input);

struct cdb_error {
	char tag[64];
	char msg[256];
	enum cdb_rpc_err rpc_err;
};
bool cdb_check_error(json_t *result,
		     json_t *error,
		     struct cdb_error *cdb_err,
		     json_t **jerr_out);

struct cdb_sock *cdb_get_csock(struct confdb *cdb);
int cdb_get_fd(struct confdb *cdb);
int cdb_prepare_select(struct confdb *cdb, fd_set *fds, struct timeval *to);
int cdb_dispatch(struct confdb *cdb, fd_set *fds);
int cdb_check_timeouts(struct confdb *cdb);
int cdb_has_egress_reqs(struct confdb *cdb);
int cdb_wait_for_responses(struct confdb *cdb);

struct confdb *cdb_create_headless(void);
struct confdb *cdb_create(void);

void cdb_destroy(struct confdb *cdb);

int cdb_add_extension(struct confdb *cdb, const char *name,
		      const struct cdb_rpc_notifier rpc[],
		      const struct cdb_data_notifier data[],
		      const void *handler_data);
void cdb_remove_extension(struct confdb *cdb, const char *name);
int cdb_ext_inject_patch(struct confdb *cdb, const char *name, json_t *jpatch);
int cdb_ext_inject_rpc(struct confdb *cdb, const char *name,
		       const char *target, json_t *input);

void cdb_set_default_rsp_handler(struct confdb *cdb,
				 cdb_rpc_rsp_handler_t handler, const void *data);

struct ly_ctx *cdb_get_lyctx(struct confdb *cdb);
const char *cdb_op2str(enum cdb_op_type op_type);
const char *cdb_where2str(enum cdb_op_where where);
const char *cdb_add_string(struct confdb *cdb, const char *str);
void cdb_del_string(struct confdb *cdb, const char *str);

#endif
