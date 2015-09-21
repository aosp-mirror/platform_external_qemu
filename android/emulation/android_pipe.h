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
#pragma once

#include "android/utils/compiler.h"
#include "android/utils/stream.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// TECHNICAL NOTE:
//
// An Android pipe is a very fast communication channel between the guest
// system and the emulator program.
//
// To open a new pipe to the emulator, a guest client will do the following:
//
//     fd = open("/dev/qemu_pipe", O_RDWR);
//     ret = write(fd, pipeName, strlen(pipeName) + 1);
//     if (ret < 0) {
//         // something bad happened, see errno
//     }
//
//     now read()/write() to communicate with <pipeName> service in the
//     emulator. select() and non-blocking i/o will also work properly.
//

// The Goldfish pipe virtual device is in charge of communicating with the
// kernel to manage pipe operations.
//
//
//
//
// This header provides the interface used by pipe services in the emulator
// to receive new client connection and deal with them.
//
//
// 1/ Call android_pipe_service_add() to register a new pipe service by name.
//    This must provide a pointer to a series of functions that will be called
//    during normal pipe operations.
//
// 2/ When a client connects to the service, the 'init' callback will be called
//    to create a new service-specific client identifier (which must returned
//    by the function).
//
// 3/ Call android_pipe_close() to force the closure of a given pipe.
//
// 4/ Call android_pipe_wake() to signal a change of state to the pipe.
//
//

// Buffer descriptor for sendBuffers() and recvBuffers() callbacks.
typedef struct AndroidPipeBuffer {
    uint8_t* data;
    size_t size;
} AndroidPipeBuffer;

// The set of methods implemented by an AndroidPipe instance.
// All these methods are called at runtime by the virtual device
// implementation. Note that transfers are always initiated by the virtual
// device through the sendBuffers() and recvBuffers() callbacks.
//
// More specifically:
//
// - When a pipe has data for the guest, it should signal it by calling
//   'android_pipe_wake(pipe, PIPE_WAKE_READ)'. The guest kernel will be
//   signaled and will later initiate a recvBuffers() call.
//
// - When a pipe is ready to accept data from the guest, it should signal
//   it by calling 'android_pipe_wake(pipe, PIPE_WAKE_WRITE)'. The guest
//   kernel will be signaled, and will later initiate a sendBuffers()
//   call when the guest client writes data to the pipe.
//
// - Finally, the guest kernel can signal whether it wants to be signaled
//   on read or write events by calling the wakeOn() callback.
//
// - When the emulator wants to close a pipe, it shall call
//   android_pipe_close(). This signals the guest kernel which will later
//   initiate a close() call.
//
typedef struct AndroidPipeFuncs {
    // Call to open a new pipe instance. |hwpipe| is a device-side view of the pipe
    // that must be passed to android_pipe_xxx() functions. |pipeOpaque| is the
    // 'opaque' value passed to android_pipe_add_type(), and |args| is either NULL
    // or parameters passed to the service when opening the connection.
    // Return a new instance, or NULL on error.
    void* (*init)(void* hwpipe, void* pipeOpaque, const char* args);

    // Called when the guest kernel has finally closed a pipe connection.
    // This is the only place one should release/free the instance, but never
    // call this directly, use android_pipe_close() instead.
    void (*close)(void* pipe);

    // Called when the guest wants to write data to the |pipe|.
    // |buffers| points to an array of |numBuffers| descriptors that describe
    // where to copy the data from. Return the number of bytes that were
    // actually transferred, 0 for EOF status, or a negative error value
    // otherwise, including PIPE_ERROR_AGAIN to indicate that the emulator
    // is not ready yet to receive data.
    int (*sendBuffers)(void* pipe,
                       const AndroidPipeBuffer* buffers,
                       int numBuffers);

    // Called when the guest wants to read data from the |pipe|.
    // |buffers| points to an array of |numBuffers| descriptors that describe
    // where to copy the data to. Return the number of bytes that were
    // actually transferred, 0 for EOF status, or a negative error value
    // otherwise, including PIPE_ERROR_AGAIN to indicate that the emulator
    // doesn't have data for the guest yet.
    int (*recvBuffers)(void* pipe,
                       AndroidPipeBuffer* buffers,
                       int numBuffers);

    // Called when guest wants to poll the read/write status for the |pipe|.
    // Should return a combination of PIPE_POLL_XXX flags.
    unsigned (*poll)(void* pipe);

    // Called to signal that the guest wants to be woken when the set of
    // PIPE_WAKE_XXX bit-flags in |flags| occur. When the condition occurs,
    // then the |pipe| implementation shall call android_pipe_wake().
    void (*wakeOn)(void* pipe, int flags);

    // Called to save the |pipe|'s state to a Stream, i.e. when saving
    // snapshots.
    void (*save)(void* pipe, Stream* file);

    // Called to load the sate of a pipe from a QEMUFile. This will always
    // correspond to the state of the pipe as saved by a previous call to
    // the 'save' method. Can be NULL to indicate that the pipe state cannot
    // be loaded. In this case, the emulator will automatically force-close
    // it. Parameters are similar to init(), with the addition of |file|
    // which is the input stream.
    void* (*load)(void* hwpipe, void* pipeOpaque, const char* args,
                  Stream* file);

} AndroidPipeFuncs;

/* Register a new pipe handler type. 'pipeOpaque' is passed directly
 * to 'init() when a new pipe is connected to.
 */
extern void  android_pipe_add_type(const char* pipeName,
                                   void* pipeOpaque,
                                   const AndroidPipeFuncs*  pipeFuncs);

// A set of functions that must be implemented by the virtual device
// implementation. Used with call android_pipe_set_hw_funcs().
typedef struct AndroidPipeHwFuncs {
    // Called from the host to signal that it wants to close a pipe.
    // |hwpipe| is the device-side view of the pipe.
    void (*closeFromHost)(void* hwpipe);

    // Called to signal that the host has data for the guest (PIPE_WAKE_READ),
    // or that the host is ready to accept data from the guest
    // (PIPE_WAKE_WRITE). |hwpipe| is the device-side view of the pipe,
    // and |flags| is a combination of PIPE_WAKE_READ and PIPE_WAKE_WRITE.
    void (*signalWake)(void* hwpipe, unsigned flags);

} AndroidPipeHwFuncs;

extern void android_pipe_set_hw_funcs(const AndroidPipeHwFuncs* hw_funcs);

/* This tells the guest system that we want to close the pipe and that
 * further attempts to read or write to it will fail. This will not
 * necessarily call the 'close' callback immediately though.
 *
 * This will also wake-up any blocked guest threads waiting for i/o.
 */
extern void android_pipe_close(void* hwpipe);

/* Signal that the pipe can be woken up. 'flags' must be a combination of
 * PIPE_WAKE_READ and PIPE_WAKE_WRITE.
 */
extern void android_pipe_wake(void* hwpipe, unsigned flags);

/* Possible status values used to signal errors - see qemu_pipe_error_convert */
#define PIPE_ERROR_INVAL       -1
#define PIPE_ERROR_AGAIN       -2
#define PIPE_ERROR_NOMEM       -3
#define PIPE_ERROR_IO          -4

/* Bit-flags used to signal events from the emulator */
#define PIPE_WAKE_CLOSED       (1 << 0)  /* emulator closed pipe */
#define PIPE_WAKE_READ         (1 << 1)  /* pipe can now be read from */
#define PIPE_WAKE_WRITE        (1 << 2)  /* pipe can now be written to */

/* List of bitflags returned in status of CMD_POLL command */
#define PIPE_POLL_IN   (1 << 0)
#define PIPE_POLL_OUT  (1 << 1)
#define PIPE_POLL_HUP  (1 << 2)

// Called from the hardware-side.
extern void* android_pipe_new(void* hwpipe);
extern void android_pipe_free(void* pipe);
extern void android_pipe_save(void* pipe, Stream* file);
extern void* android_pipe_load(Stream* file, void* hwpipe, char* force_close);
extern void* android_pipe_load_legacy(Stream* file, void* hwpipe,
                                      uint64_t* channel, unsigned char* wakes,
                                      unsigned char* closed, char* force_close);
extern unsigned android_pipe_poll(void* pipe);
extern int android_pipe_recv(void* pipe, AndroidPipeBuffer* buffers, int numBuffers);
extern int android_pipe_send(void* pipe, const AndroidPipeBuffer* buffer, int numBuffer);
extern void android_pipe_wake_on(void* pipe_, unsigned wakes);

ANDROID_END_HEADER
