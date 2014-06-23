/*
 * ARM Android emulator adb backend
 *
 * Copyright (c) 2014 Linaro Limited
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
 * Handle connections to the qemud:adb pipe from Android guests and route
 * traffic over this pipe to the host adb server connecting to the qemu
 * process on a tcp socket.
 */

#include "hw/misc/android_pipe.h"


// #define DEBUG_ADB_DEBUG

#ifdef DEBUG_ADB_DEBUG
#define DPRINTF(fmt, ...) \
do { fprintf(stderr, "adb_debug: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

static bool _pipe_backend_initialized = false;

typedef struct {
    void*     hwpipe;
} adb_dbg_pipe;

static void* adb_dbg_pipe_init(void *hwpipe,void *opaque, const char *args)
{
    adb_dbg_pipe *apipe;

    DPRINTF("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    apipe = g_malloc0(sizeof(adb_dbg_pipe));
    apipe->hwpipe = hwpipe;
    return apipe;
}

static void adb_dbg_pipe_close(void *opaque)
{
    adb_dbg_pipe *apipe = opaque;

    DPRINTF("%s: hwpipe=%p", __FUNCTION__, apipe->hwpipe);
    g_free(apipe);
}

static unsigned pipe_buffers_len(const AndroidPipeBuffer *buffers, int cnt)
{
    unsigned len = 0;
    while (cnt > 0) {
        len += buffers[0].size;
        cnt --;
        buffers++;
    }
    return len;
}

/* Guest is sending us some ADB traces, print it on stderr */
static int adb_dbg_pipe_send(void *opaque, const AndroidPipeBuffer *buffers,
                             int cnt)
{
    int ret = 0;
    char *data;
    unsigned total_len = pipe_buffers_len(buffers, cnt);

    DPRINTF("%s: something is coming over the wire....\n", __func__);

    if (total_len > 4 * 4096) {
        return total_len;  /* ignore outrageous requests */
    }

    data = g_malloc(total_len);

    while (cnt > 0) {
        memcpy(data, buffers[0].data, buffers[0].size);
        data += buffers[0].size;
        ret += buffers[0].size;

        cnt--;
        buffers++;
    }

    fprintf(stderr, "ADB: %s", data);

    return ret;
}

/* Guest is reading data: Act as the zero pipe and just return a bunch of
 * zeros.
 */
static int adb_dbg_pipe_recv(void *opaque, AndroidPipeBuffer *buffers,
                             int cnt)
{
    int ret = 0;

    while (cnt > 0) {
        ret += buffers[0].size;
        memset(buffers[0].data, 0, buffers[0].size);
        buffers++;
        cnt--;
    }
    return ret;
}

static unsigned adb_dbg_pipe_poll(void *opaque)
{
    return PIPE_POLL_IN | PIPE_POLL_OUT;
}

static void
adb_dbg_pipe_wake_on(void *opaque, int flags)
{
    /* nothing to do here, we never block */
}

static const AndroidPipeFuncs adb_dbg_pipe_funcs = {
    adb_dbg_pipe_init,
    adb_dbg_pipe_close,
    adb_dbg_pipe_send,
    adb_dbg_pipe_recv,
    adb_dbg_pipe_poll,
    adb_dbg_pipe_wake_on,
};

void android_adb_dbg_backend_init(void)
{
    if (_pipe_backend_initialized) {
        return;
    }

    android_pipe_add_type("qemud:adb-debug", NULL, &adb_dbg_pipe_funcs);
    _pipe_backend_initialized = true;
}
