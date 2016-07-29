#ifndef MONITOR_H
#define MONITOR_H

#include "qemu-common.h"
#include "qapi/qmp/qerror.h"
#include "qapi/qmp/qdict.h"
#include "block/block.h"
#include "qemu/readline.h"

extern Monitor *cur_mon;
extern Monitor *default_mon;

typedef void (MonitorCompletion)(void *opaque, QObject *ret_data);

typedef union cmd_table_t {
    struct mon_cmd_t *static_table;
    GArray           *dynamic_table;
} cmd_table_t;

typedef struct mon_cmd_t {
    const char *name;
    const char *args_type;
    const char *params;
    const char *help;
    void (*user_print)(Monitor *mon, const QObject *data);
    union {
        void (*cmd)(Monitor *mon, const QDict *qdict);
        int  (*cmd_new)(Monitor *mon, const QDict *params, QObject **ret_data);
        int  (*cmd_async)(Monitor *mon, const QDict *params,
                          MonitorCompletion *cb, void *opaque);
    } mhandler;
    int flags;
    /* @sub_cmd is a list of 2nd level of commands. If it do not exist,
     * mhandler should be used. If it exist, sub_table[?].mhandler should be
     * used, and mhandler of 1st level plays the role of help
     * function.
     *
     * .hx defined sub_cmds can only be static.
     */
    cmd_table_t sub_cmds;

    void (*command_completion)(ReadLineState *rs, int nb_args, const char *str);
} mon_cmd_t;

/* flags for monitor_init */
#define MONITOR_IS_DEFAULT    0x01
#define MONITOR_USE_READLINE  0x02
#define MONITOR_USE_CONTROL   0x04
#define MONITOR_USE_PRETTY    0x08
#define MONITOR_DYNAMIC_CMDS  0x10

/* flags for monitor commands */
#define MONITOR_CMD_ASYNC       0x0001

int monitor_cur_is_qmp(void);

Monitor * monitor_init(CharDriverState *chr, int flags);
void monitor_add_command(Monitor *mon, mon_cmd_t *cmd);

int monitor_suspend(Monitor *mon);
void monitor_resume(Monitor *mon);

int monitor_read_bdrv_key_start(Monitor *mon, BlockDriverState *bs,
                                BlockCompletionFunc *completion_cb,
                                void *opaque);
int monitor_read_block_device_key(Monitor *mon, const char *device,
                                  BlockCompletionFunc *completion_cb,
                                  void *opaque);

int monitor_get_fd(Monitor *mon, const char *fdname, Error **errp);
int monitor_handle_fd_param(Monitor *mon, const char *fdname);
int monitor_handle_fd_param2(Monitor *mon, const char *fdname, Error **errp);

void monitor_vprintf(Monitor *mon, const char *fmt, va_list ap)
    GCC_FMT_ATTR(2, 0);
void monitor_printf(Monitor *mon, const char *fmt, ...) GCC_FMT_ATTR(2, 3);
void monitor_flush(Monitor *mon);
int monitor_set_cpu(int cpu_index);
int monitor_get_cpu_index(void);

void monitor_set_error(Monitor *mon, QError *qerror);
void monitor_read_command(Monitor *mon, int show_prompt);
ReadLineState *monitor_get_rs(Monitor *mon);
int monitor_read_password(Monitor *mon, ReadLineFunc *readline_func,
                          void *opaque);

int qmp_qom_set(Monitor *mon, const QDict *qdict, QObject **ret);

int qmp_qom_get(Monitor *mon, const QDict *qdict, QObject **ret);
int qmp_object_add(Monitor *mon, const QDict *qdict, QObject **ret);
void object_add(const char *type, const char *id, const QDict *qdict,
                Visitor *v, Error **errp);

AddfdInfo *monitor_fdset_add_fd(int fd, bool has_fdset_id, int64_t fdset_id,
                                bool has_opaque, const char *opaque,
                                Error **errp);
int monitor_fdset_get_fd(int64_t fdset_id, int flags);
int monitor_fdset_dup_fd_add(int64_t fdset_id, int dup_fd);
void monitor_fdset_dup_fd_remove(int dup_fd);
int monitor_fdset_dup_fd_find(int dup_fd);

#endif /* !MONITOR_H */
