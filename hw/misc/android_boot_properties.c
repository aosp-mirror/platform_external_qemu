/*
 * Android emulator boot properties backend
 *
 * Copyright 2015 The Android Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Handles the 'boot-properties' service used to inject system properties
 * at boot time.
 */

#include <glib.h>
#include <stdio.h>

#include "qemu-common.h"
#include "hw/misc/android_boot_properties.h"
#include "hw/misc/android_pipe.h"
#include "hw/misc/android_qemud.h"

//#define DEBUG_BOOT_PROPERTIES

#ifdef DEBUG_BOOT_PROPERTIES
#define DPRINTF(fmt, ...) \
do { fprintf(stderr, "%s: " fmt, __func__, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

static GPtrArray *all_boot_properties;

typedef struct {
    void *hwpipe;
    GPtrArray *send_data;
    GString *send_outstanding;
    GPtrArray *recv_data;
    GString *recv_outstanding;
} BootPropPipe;

static ssize_t boot_prop_pipe_have_data(BootPropPipe *props,
                                        const gchar *buf)
{
    int len = strlen(buf);

    if (!strcmp(buf, "list")) {
        GPtrArray *properties = all_boot_properties;
        if (properties) {
            guint count = properties->len;
            guint n;
            for (n = 0; n < count; n += 2) {
                const gchar *key = g_ptr_array_index(properties, n);
                const gchar *value = g_ptr_array_index(properties, n + 1);
                gchar *line = g_strdup_printf("%s=%s", key, value);
                g_ptr_array_add(props->send_data, line);
            }
        }
        g_ptr_array_add(props->send_data, g_strdup(""));
        android_pipe_wake(props->hwpipe, PIPE_WAKE_READ);
    } else {
        DPRINTF("bad command [%s] expected [%s]", buf, "list");
    }
    return len;
}

static void *boot_prop_pipe_init(void *hwpipe,
                                 void *opaque,
                                 const char *args)
{
    BootPropPipe *pipe;

    pipe = g_malloc0(sizeof(*pipe));
    pipe->hwpipe = hwpipe;
    pipe->send_data = g_ptr_array_new_full(8, g_free);
    pipe->recv_data = g_ptr_array_new_full(1, g_free);
    pipe->send_outstanding = g_string_sized_new(128);
    pipe->recv_outstanding = g_string_sized_new(8);

    return pipe;
}

static void boot_prop_pipe_close(void *opaque)
{
    BootPropPipe *pipe = opaque;
    g_ptr_array_free(pipe->send_data, TRUE);
    g_ptr_array_free(pipe->recv_data, TRUE);
    g_string_free(pipe->send_outstanding, TRUE);
    g_string_free(pipe->recv_outstanding, TRUE);
    g_free(pipe);
}

static void boot_prop_pipe_wake(void *opaque, int flags)
{
    BootPropPipe *pipe = opaque;

    if (flags & PIPE_WAKE_READ
        && (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0)) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }

    if (flags & PIPE_WAKE_WRITE) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
    }
}

static int boot_prop_pipe_recv(void *opaque,
                                AndroidPipeBuffer *buffers,
                                int cnt)
{
    BootPropPipe *pipe = opaque;

    if (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0) {
        return qemud_pipe_write_buffers(pipe->send_data,
                                        pipe->send_outstanding,
                                        buffers, cnt);
    } else {
        return PIPE_ERROR_AGAIN;
    }
}

static int boot_prop_pipe_send(void *opaque,
                                const AndroidPipeBuffer *buffers,
                                int cnt)
{
    BootPropPipe *pipe = opaque;
    int consumed = qemud_pipe_read_buffers(buffers, cnt,
                                           pipe->recv_data,
                                           pipe->recv_outstanding);

    while (pipe->recv_data->len > 0) {
        gchar *msg = g_ptr_array_index(pipe->recv_data, 0);
        boot_prop_pipe_have_data(pipe, msg);
        g_ptr_array_remove(pipe->recv_data, msg);
    }

    return consumed;
}

static unsigned boot_prop_pipe_poll(void *opaque)
{
    BootPropPipe *pipe = opaque;
    unsigned flags = 0;

    if (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0) {
        flags |= PIPE_POLL_IN;
    }
    flags |= PIPE_POLL_OUT;
    return flags;
}

static const AndroidPipeFuncs boot_prop_pipe_funcs = {
    boot_prop_pipe_init,
    boot_prop_pipe_close,
    boot_prop_pipe_send,
    boot_prop_pipe_recv,
    boot_prop_pipe_poll,
    boot_prop_pipe_wake,
};

void android_boot_properties_init(void)
{
    android_pipe_add_type("qemud:boot-properties", NULL, &boot_prop_pipe_funcs);
}

void android_boot_property_add(const char* name,
                               const char* value) {
    if (!all_boot_properties) {
        all_boot_properties = g_ptr_array_new_full(8, g_free);
    }
    g_ptr_array_add(all_boot_properties, g_strdup(name));
    g_ptr_array_add(all_boot_properties, g_strdup(value));
}
