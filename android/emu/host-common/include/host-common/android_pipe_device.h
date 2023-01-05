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

#include "android/hw-export.h"
#include "android/emulation/android_pipe_common.h"
#include "android/utils/compiler.h"
#include "android/utils/stream.h"

#include <stdbool.h>
#include <stdint.h>

#define ANDROID_PIPE_DEVICE_EXPORT extern AEMU_HW_API

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
// Each pipe connection corresponds to two objects:
//
//   - A hardware-side opaque object (implemented by the virtual pipe device)
//     identified here as a |hwpipe| value.
//
//  - A host-specific opaque object, identified here either as |host-pipe|.
//
//       (virtual device) |hwpipe|  <----->  |host-pipe|   (emulator)
//
// Expected usage is the following:
//
// 1/ During setup, the virtual device should call android_pipe_set_hw_funcs()
//    to point to callbacks it provides to implement a few functions called
//    from the host, later at runtime.
//
// 2/ During setup, the emulator should register host pipe service instances,
//    each one identified by a name, before the VM starts.
//
// 3/ At runtime, when a guest opens /dev/goldfish_pipe, this ends up creating
//    a new |hwpipe| object in the virtual device, which will then call
//    android_pipe_guest_open() to create a corresponding |host-pipe|
//    object.
//
//    Note that at this point, the |host-pipe| corresponds to a special
//    host-side pipe state that waits for the name of the pipe service to
//    connect to, and potential arguments. This is called a |connect-pipe|
//    here:
//
//                     android_pipe_guest_open()
//         |hwpipe|  <--------------------------->  |connect-pipe|
//
// 4/ When the pipe service name is fully written, the |connect-pipe| finds a
//    service registered for it, and calls its ::init() callback to create
//    a new |service-pipe| instance.
//
//         |hwpipe|  <--------------------------->  |connect-pipe|
//
//                                                  |service-pipe|
//
// 5/ It then calls the AndroidPipeHwFuncs::resetPipe() callback provided
//    by the virtual device to reattach the |hwpipe| to the |service-pipe|
//    then later delete the now-useless |connect-pipe|.
//
//         |hwpipe|  <------------+                 |connect-pipe|
//                                |
//                                +---------------->|service-pipe|
//
// VERY IMPORTANT NOTE:
//
// All operations should happen on the thread that currently holds the
// global VM state lock (see android/emulation/VmLock.h). It's up to
// the host implementation to ensure that this is always the case.
//

////////////////////////////////////////////////////////////////////////////
//
// The following functions are called from the virtual device only and are
// implemented by AndroidEmu. They will always be called from the device thread
// context as well.

// Open a new Android pipe. |hwpipe| is a unique pointer value identifying the
// hardware-side view of the pipe, and will be passed to the 'init' callback
// from AndroidPipeHwFuncs. This returns a new |internal-pipe| value that is
// only used by the virtual device to call android_pipe_xxx() functions below.
ANDROID_PIPE_DEVICE_EXPORT void* android_pipe_guest_open(
    void* hwpipe, void(*check_pipe_name)(const char* name));

// Similar to android_pipe_guest_open(), but has a flags parameter that is used
// to communicate unique transport properties about the pipe.
ANDROID_PIPE_DEVICE_EXPORT void* android_pipe_guest_open_with_flags(
    void* hwpipe, uint32_t flags, void(*check_pipe_name)(const char* name));

// Close and free an Android pipe. |pipe| must be the result of a previous
// android_pipe_guest_open() call or the second parameter to
// android_pipe_reset().
// This must *not* be called from the host side, see android_pipe_host_close()
// instead.
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_close(
    void* internal_pipe, PipeCloseReason reason);

// Hooks for a start/end of save/load operations, called once per each snapshot.
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_pre_load(Stream* file);
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_post_load(Stream* file);
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_pre_save(Stream* file);
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_post_save(Stream* file);

// Save the state of an Android pipe to a stream. |internal_pipe| is the pipe
// instance from android_pipe_guest_open() or android_pipe_guest_load(), and
// |file| is the
// output stream.
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_save(
    void* internal_pipe, Stream* file);

// Load the state of an Android pipe from a stream. |file| is the input stream,
// |hwpipe| is the hardware-side pipe descriptor. On success, return a new
// internal pipe instance (similar to one returned by
// android_pipe_guest_open()), and
// sets |*force_close| to 1 to indicate that the pipe must be force-closed
// just after its load/creation (only useful for certain services that can't
// preserve state into streams). Return NULL on faillure.
ANDROID_PIPE_DEVICE_EXPORT void* android_pipe_guest_load(
    Stream* file, void* hwpipe, char* force_close);

// Similar to android_pipe_guest_load(), but this function is only called from
// the
// QEMU1 virtual pipe device, to support legacy snapshot formats. It can be
// ignored by the QEMU2 virtual device implementation.
//
// The difference is that this must also read the hardware-specific state
// fields, and store them into |*channel|, |*wakes| and |*closed| on
// success.
ANDROID_PIPE_DEVICE_EXPORT void* android_pipe_guest_load_legacy(
    Stream* file, void* hwpipe, uint64_t* channel,
    unsigned char* wakes, unsigned char* closed, char* force_close);

// Call the poll() callback of the client associated with |pipe|.
ANDROID_PIPE_DEVICE_EXPORT unsigned android_pipe_guest_poll(void* internal_pipe);

// Call the recvBuffers() callback of the client associated with |pipe|.
ANDROID_PIPE_DEVICE_EXPORT int android_pipe_guest_recv(
    void* internal_pipe, AndroidPipeBuffer* buffers, int numBuffers);

// Call the sendBuffers() callback of the client associated with |pipe|.
ANDROID_PIPE_DEVICE_EXPORT int android_pipe_guest_send(
    void** internal_pipe, const AndroidPipeBuffer* buffer, int numBuffer);

// Call the wakeOn() callback of the client associated with |pipe|.
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_guest_wake_on(
    void* internal_pipe, unsigned wakes);

// Utility functions to look up pipe instances and ids.
ANDROID_PIPE_DEVICE_EXPORT int android_pipe_get_id(void* internal_pipe);
ANDROID_PIPE_DEVICE_EXPORT void* android_pipe_lookup_by_id(int id);

// Each pipe type should regiter its callback to lookup instances by it.
// |android_pipe_lookup_by_id| will go through the list of callbacks and
// will check if no more than one value is returned, it will crash otherwise.
// The |tag| argiment will be used in the crash message.
ANDROID_PIPE_DEVICE_EXPORT void android_pipe_append_lookup_by_id_callback(
    void*(*callback)(int), const char* tag);
