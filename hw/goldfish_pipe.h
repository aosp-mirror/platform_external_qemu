/* Copyright (C) 2011 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_PIPE_H
#define _HW_GOLDFISH_PIPE_H

#include <stdint.h>
#include "android/hw-qemud.h"

/* The following functions should called from hw/goldfish_trace.c and are
 * used to implement QEMUD 'fast-pipes' in the Android emulator.
 */
extern void      goldfish_pipe_thread_death(int  tid);
extern void      goldfish_pipe_write(int tid, int offset, uint32_t value);
extern uint32_t  goldfish_pipe_read(int tid, int offset);

/* The following definitions are used to define a "pipe handler" type.
 * Each pipe handler manages a given, named pipe type, and must provide
 * a few callbacks that will be used at appropriate times:
 *
 * - init ::
 *       is called when a guest client has connected to a given
 *       pipe. The function should return an opaque pointer that
 *       will be passed as the first parameter to other callbacks.
 *
 *       Note: pipeOpaque is the value that was passed to
 *             goldfish_pipe_add_type() to register the pipe handler.
 *
 * - close ::
 *       is called when the pipe is closed.(either from the guest, or
 *       when the handler itself calls qemud_client_close() on the
 *       corresponding QemudClient object passed to init()).
 *
 * - sendBuffers ::
 *       is called when the guest is sending data through the pipe. This
 *       callback receives a list of buffer descriptors that indicate
 *       where the data is located in memory.
 *
 *       Must return 0 on success, or -1 on failure, with errno of:
 *
 *           ENOMEM -> indicates that the message is too large
 *           EAGAIN -> indicates that the handler is not ready
 *                     to accept the message yet.
 *
 * - recvBuffers ::
 *       Is called when the guest wants to receive data from the pipe.
 *       The caller provides a list of memory buffers to put the data into.
 *
 *       Must return the size of the incoming data on success, or -1
 *       on error, with errno of:
 *
 *           ENOMEM -> buffer too small to receive the message
 *           EAGAIN -> no incoming data yet
 *
 * - wakeOn ::
 *       is called to indicate that the guest wants to be waked when the
 *       pipe becomes able to either receive data from the guest, or send it
 *       new incoming data. It is the responsability of the pipe handler to
 *       signal the corresponding events by sending a single byte containing
 *       QEMUD_PIPE_WAKE_XXX bit flags through qemud_client_send() to do so.
 */

enum {
    QEMUD_PIPE_WAKE_ON_SEND = (1 << 0),
    QEMUD_PIPE_WAKE_ON_RECV = (1 << 1),
};

/* Buffer descriptor for sendBuffers() and recvBuffers() callbacks */
typedef struct GoldfishPipeBuffer {
    uint8_t*  data;
    size_t    size;
} GoldfishPipeBuffer;

/* Pipe handler funcs */
typedef struct {
    void*        (*init)( QemudClient*  client, void* pipeOpaque );
    void         (*close)( void* opaque );
    int          (*sendBuffers)( void* opaque, const GoldfishPipeBuffer*  buffers, int numBuffers );
    int          (*recvBuffers)( void* opaque, GoldfishPipeBuffer* buffers, int numBuffers );
    void         (*wakeOn)( void* opaque, int flags );
} QemudPipeHandlerFuncs;

/* Register a new pipe handler type. 'pipeOpaque' is passed directly
 * to 'init() when a new pipe is connected to.
 */
extern void  goldfish_pipe_add_type(const char*                   pipeName,
                                     void*                         pipeOpaque,
                                     const QemudPipeHandlerFuncs*  pipeFuncs );

#endif /* _HW_GOLDFISH_PIPE_H */
