/* Copyright (C) 2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "qemu-common.h"
#include "android/globals.h"
#include "android/android.h"
#include "android/looper.h"
#include "android/async-utils.h"
#include "android/sync-utils.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"
#include "android/protocol/ui-commands.h"
#include "android/protocol/ui-commands-proxy.h"

/* Descriptor for the UI commands proxy. */
typedef struct UICmdProxy {
    /* I/O associated with this descriptor. */
    LoopIo          io;

    /* Looper associated with this descriptor. */
    Looper*         looper;

    /* Writer to send UI commands. */
    SyncSocket*     sync_writer;

    /* Socket descriptor for this service. */
    int             sock;
} UICmdProxy;

/* One and only one UICmdProxy instance. */
static UICmdProxy    _proxy;

/* Implemented in android/console.c */
extern void destroy_uicmd_proxy(void);

/* Sends request to the UI client.
 * Param:
 *  cmd_type, cmd_param, cmd_param_size - Define core request to send.
 * Return:
 *  0 on success, or < 0 on failure.
 */
static int
_uicmd_send_request(uint8_t cmd_type, void* cmd_param, uint32_t cmd_param_size)
{
    UICmdHeader header;
    int status = syncsocket_start_write(_proxy.sync_writer);
    if (!status) {
        // Initialize and send the header.
        header.cmd_type = cmd_type;
        header.cmd_param_size = cmd_param_size;
        status = syncsocket_write(_proxy.sync_writer, &header, sizeof(header),
                                  _get_transfer_timeout(sizeof(header)));
        // If there are commad parameters, send them too.
        if (status > 0 && cmd_param != NULL && cmd_param_size > 0) {
            status = syncsocket_write(_proxy.sync_writer, cmd_param,
                                      cmd_param_size,
                                      _get_transfer_timeout(cmd_param_size));
        }
        status = syncsocket_result(status);
        syncsocket_stop_write(_proxy.sync_writer);
    }
    if (status < 0) {
        derror("Send UI proxy command %d failed: %s\n", cmd_type, errno_str);
    }
    return status;
}

/*
 * Asynchronous I/O callback for UICmdProxy instance.
 * We expect this callback to be called only on UI detachment condition. In this
 * case the event should be LOOP_IO_READ, and read should fail with errno set
 * to ECONNRESET.
 * Param:
 *  opaque - UICmdProxy instance.
 */
static void
_uicmd_io_func(void* opaque, int fd, unsigned events)
{
    UICmdProxy* uicmd = (UICmdProxy*)opaque;
    AsyncReader reader;
    AsyncStatus status;
    uint8_t read_buf[1];

    if (events & LOOP_IO_WRITE) {
        derror("Unexpected LOOP_IO_WRITE in _uicmd_io_func\n");
        return;
    }

    // Try to read
    asyncReader_init(&reader, read_buf, sizeof(read_buf), &uicmd->io);
    status = asyncReader_read(&reader, &uicmd->io);
    // We expect only error status here.
    if (status != ASYNC_ERROR) {
        derror("Unexpected read status %d in _uicmd_io_func\n", status);
        return;
    }
    // We expect only socket disconnection here.
    if (errno != ECONNRESET) {
        derror("Unexpected read error %d (%s) in _uicmd_io_func\n",
               errno, errno_str);
        return;
    }

    // Client got disconnectted.
    destroy_uicmd_proxy();
}

int
uiCmdProxy_create(int fd)
{
    // Initialize _proxy instance.
    _proxy.sock = fd;
    _proxy.looper = looper_newCore();
    loopIo_init(&_proxy.io, _proxy.looper, _proxy.sock, _uicmd_io_func, &_proxy);
    loopIo_wantRead(&_proxy.io);
    _proxy.sync_writer = syncsocket_init(fd);
    if (_proxy.sync_writer == NULL) {
        derror("Unable to initialize UICmdProxy writer: %s\n", errno_str);
        return -1;
    }
    return 0;
}

void
uiCmdProxy_destroy()
{
    if (_proxy.looper != NULL) {
        // Stop all I/O that may still be going on.
        loopIo_done(&_proxy.io);
        looper_free(_proxy.looper);
        _proxy.looper = NULL;
    }
    if (_proxy.sync_writer != NULL) {
        syncsocket_close(_proxy.sync_writer);
        syncsocket_free(_proxy.sync_writer);
    }
    _proxy.sock = -1;
}

int
uicmd_set_window_scale(double scale, int is_dpi)
{
    UICmdSetWindowsScale cmd;
    cmd.scale = scale;
    cmd.is_dpi = is_dpi;
    return _uicmd_send_request(AUICMD_SET_WINDOWS_SCALE, &cmd, sizeof(cmd));
}
