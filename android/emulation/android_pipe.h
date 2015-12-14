/* Copyright 2015 The Android Open Source Project
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
//     ret = write(fd, pipeServiceName, strlen(pipeServiceName) + 1);
//     if (ret < 0) {
//         // something bad happened, see errno
//     }
//
// Where 'pipeServiceName' is a string that looke like "pipe:<service>"
// or "pipe:<service>:<args>", terminated by a zero.
//
//     now read()/write() to communicate with <pipeServiceName> service in the
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
//    to create a new service-specific client identifier (which must be
//    returned by the function).
//
// 3/ Call android_pipe_close() to force the closure of a given pipe. Note
//    that this takes the 'hwpipe' parameter that was passed to init().
//
// 4/ Call android_pipe_wake() to signal a change of state to the pipe.
//    Note that this also takes the 'hwpipe' parameter from init().
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
    // Call to open a new pipe instance. |hwpipe| is a device-side view of the
    // pipe that must be passed to android_pipe_wake() and android_pipe_close()
    // functions. |serviceOpaque| is the parameter passed to
    // android_pipe_add_type(), and |args| is either NULL
    // or parameters passed to the service when opening the connection.
    // Return a new pipe client instance, or NULL on error.
    void* (*init)(void* hwpipe, void* serviceOpaque, const char* args);

    // Called when the guest kernel has finally closed a pipe connection.
    // This is the only place one should release/free the instance, but never
    // call this directly, use android_pipe_close() instead. |pipe| is a client
    // instance returned by init() or load().
    void (*close)(void* pipe);

    // Called when the guest wants to write data to the |pipe| client instance,
    // |buffers| points to an array of |numBuffers| descriptors that describe
    // where to copy the data from. Return the number of bytes that were
    // actually transferred, 0 for EOF status, or a negative error value
    // otherwise, including PIPE_ERROR_AGAIN to indicate that the emulator
    // is not ready yet to receive data.
    int (*sendBuffers)(void* pipe,
                       const AndroidPipeBuffer* buffers,
                       int numBuffers);

    // Called when the guest wants to read data from the |pipe| client,
    // |buffers| points to an array of |numBuffers| descriptors that describe
    // where to copy the data to. Return the number of bytes that were
    // actually transferred, 0 for EOF status, or a negative error value
    // otherwise, including PIPE_ERROR_AGAIN to indicate that the emulator
    // doesn't have data for the guest yet.
    int (*recvBuffers)(void* pipe,
                       AndroidPipeBuffer* buffers,
                       int numBuffers);

    // Called when guest wants to poll the read/write status for the |pipe|
    // client. Should return a combination of PIPE_POLL_XXX flags.
    unsigned (*poll)(void* pipe);

    // Called to signal that the guest wants to be woken when the set of
    // PIPE_WAKE_XXX bit-flags in |flags| occur. When the condition occurs,
    // then the |pipe| client shall call android_pipe_wake().
    void (*wakeOn)(void* pipe, int flags);

    // Called to save the |pipe| client's state to a Stream, i.e. when saving
    // snapshots.
    void (*save)(void* pipe, Stream* file);

    // Called to load the sate of a pipe client from a Stream. This will always
    // correspond to the state of the client as saved by a previous call to
    // the 'save' method. Can be NULL to indicate that the pipe state cannot
    // be loaded. In this case, the emulator will automatically force-close
    // it. Parameters are similar to init(), with the addition of |file|
    // which is the input stream.
    void* (*load)(void* hwpipe, void* serviceOpaque, const char* args,
                  Stream* file);

} AndroidPipeFuncs;

/* Register a new pipe service type. |serviceOpaque| is passed directly
 * to 'init() when a new pipe is connected to.
 */
extern void  android_pipe_add_type(const char* serviceName,
                                   void* serviceOpaque,
                                   const AndroidPipeFuncs* pipeFuncs);

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

// Called from the virtual hardware device only.
// It is important to understand that the values returned by android_pipe_new()
// and android_pipe_load() below are not the same as the one returned by
// the AndroidPipeFuncs.init() callback.
//
// The former correspond to an opened pipe descriptor in the guest that may not
// be associated with a service yet. The latter correspond to the client-side
// state of said service.

// Open a new Android pipe. |hwpipe| is a unique pointer value identifying the
// hardware-side view of the pipe, and will be passed to the 'init' callback
// from AndroidPipeHwFuncs. This returns an opaque pointer that shall only
// be passed to other functions below. It corresponds to the state of a newly
// opened Android pipe. Initially, it is waiting for a zero-terminated service
// name to be sent from the guest. When this happens, it will call the
// corresponding AndroidPipeFuncs.init callback to create a service client
// instance.
extern void* android_pipe_new(void* hwpipe);

// Close and free an Android pipe. |pipe| must be the result of a previous
// android_pipe_new() call (and not the result of AndroidPipeFuncs.init).
extern void android_pipe_free(void* pipe);

// Save the state of an Android pipe to a stream. |pipe| is the result of a
// previous android_pipe_new(), and |file| is the output stream.
extern void android_pipe_save(void* pipe, Stream* file);

// Load the state of an Android pipe from a stream. |file| is the input stream,
// |hwpipe| is the hardware-side pipe descriptor. On success, return a new
// opaque pipe instance (similar to one retruned by android_pipe_new()), and
// sets |*force_close| to 1 to indicate that the pipe must be force-closed
// just after its load/creation (only useful for certain services that can't
// preserve state into streams). Return NULL on faillure.
extern void* android_pipe_load(Stream* file, void* hwpipe, char* force_close);

// Similar to android_pipe_load(), but this function is only called from the
// QEMU1 virtual pipe device, to support legacy snapshot formats. It can be
// ignored by the QEMU2 virtual device implementation.
//
// The difference is that this must also read the hardware-specific state
// fields, and store them into |*channel|, |*wakes| and |*closed| on
// success.
extern void* android_pipe_load_legacy(Stream* file, void* hwpipe,
                                      uint64_t* channel, unsigned char* wakes,
                                      unsigned char* closed,
                                      char* force_close);

// Call the poll() callback of the client associated with |pipe|.
extern unsigned android_pipe_poll(void* pipe);

// Call the recvBuffers() callback of the client associated with |pipe|.
extern int android_pipe_recv(void* pipe, AndroidPipeBuffer* buffers,
                             int numBuffers);

// Call the sendBuffers() callback of the client associated with |pipe|.
extern int android_pipe_send(void* pipe, const AndroidPipeBuffer* buffer,
                             int numBuffer);

// Call the wakeOn() callback of the client associated with |pipe|.
extern void android_pipe_wake_on(void* pipe_, unsigned wakes);

// As a special case, QEMU2 uses its own ADB pipe connector that uses
// raw pipe, instead of the legacy framing protocol defined by qemud:adb.
// Call this function at initialization time to enable or disable this
// mode. NOTE: By default, this feature is enabled to ensure the QEMU2
// code continues to compile and work as expected. The QEMU1 setup glue
// should call this with a value of 'false' though.
extern void android_pipe_set_raw_adb_mode(bool enabled);

ANDROID_END_HEADER
