#include "monitor/monitor.h"

#include <stdarg.h>
#include "hw/hw.h"

struct Monitor {
    CharDriverState *chr;
    int mux_out;
    // int reset_seen;
    int flags;
    // int suspend_cnt;
    uint8_t outbuf[1024];
    int outbuf_index;
    // ReadLineState *rs;
    // CPUOldState *mon_cpu;
    // BlockDriverCompletionFunc *password_completion_cb;
    // void *password_opaque;
    QLIST_ENTRY(Monitor) entry;
    //int has_quit;
#ifdef CONFIG_ANDROID
    void*            fake_opaque;
    MonitorFakeFunc  fake_func;
    int64_t          fake_count;

#endif
};

// Minimalistic / fake implementation of the QEMU Monitor.

Monitor* cur_mon;

/* Return non-zero iff we have a current monitor, and it is in QMP mode.  */
int monitor_cur_is_qmp(void)
{
    return 0;
}

Monitor*
monitor_fake_new(void* opaque, MonitorFakeFunc cb)
{
    Monitor* mon;

    assert(cb != NULL);
    mon = g_malloc0(sizeof(*mon));
    mon->fake_opaque = opaque;
    mon->fake_func   = cb;
    mon->fake_count  = 0;

    return mon;
}

int
monitor_fake_get_bytes(Monitor* mon)
{
    assert(mon->fake_func != NULL);
    return mon->fake_count;
}

void
monitor_fake_free(Monitor* mon)
{
    assert(mon->fake_func != NULL);
    free(mon);
}

/* This replaces the definition in monitor.c which is in a
 * #ifndef CONFIG_ANDROID .. #endif block.
 */
void monitor_flush(Monitor *mon)
{
    if (!mon)
        return;

    if (mon->fake_func != NULL) {
        mon->fake_func(mon->fake_opaque, (void*)mon->outbuf, mon->outbuf_index);
        mon->outbuf_index = 0;
        mon->fake_count += mon->outbuf_index;
    } else if (!mon->mux_out) {
        qemu_chr_write(mon->chr, mon->outbuf, mon->outbuf_index);
        mon->outbuf_index = 0;
    }
}

/* flush at every end of line or if the buffer is full */
static void monitor_puts(Monitor *mon, const char *str)
{
    char c;

    if (!mon)
        return;

    for(;;) {
        c = *str++;
        if (c == '\0')
            break;
        if (c == '\n')
            mon->outbuf[mon->outbuf_index++] = '\r';
        mon->outbuf[mon->outbuf_index++] = c;
        if (mon->outbuf_index >= (sizeof(mon->outbuf) - 1)
            || c == '\n')
            monitor_flush(mon);
    }
}

void monitor_vprintf(Monitor* mon, const char* fmt, va_list args) {
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, args);
    monitor_puts(mon, buf);
}

void monitor_printf(Monitor *mon, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    monitor_vprintf(mon, fmt, ap);
    va_end(ap);
}

void monitor_print_filename(Monitor *mon, const char *filename)
{
    int i;

    for (i = 0; filename[i]; i++) {
        switch (filename[i]) {
        case ' ':
        case '"':
        case '\\':
            monitor_printf(mon, "\\%c", filename[i]);
            break;
        case '\t':
            monitor_printf(mon, "\\t");
            break;
        case '\r':
            monitor_printf(mon, "\\r");
            break;
        case '\n':
            monitor_printf(mon, "\\n");
            break;
        default:
            monitor_printf(mon, "%c", filename[i]);
            break;
        }
    }
}

void monitor_protocol_event(MonitorEvent event, QObject *data)
{
    /* XXX: TODO */
}

void monitor_set_error(Monitor *mon, QError *qerror)
{
    QDECREF(qerror);
}

void monitor_init(CharDriverState* cs, int flags) {
    // Nothing.
}

/* boot_set handler */
static QEMUBootSetHandler *qemu_boot_set_handler = NULL;
static void *boot_opaque;

void qemu_register_boot_set(QEMUBootSetHandler *func, void *opaque)
{
    qemu_boot_set_handler = func;
    boot_opaque = opaque;
}
