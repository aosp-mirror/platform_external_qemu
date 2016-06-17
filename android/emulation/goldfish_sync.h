/* Copyright (C) 2016 The Android Open Source Project
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

#include <inttypes.h>

ANDROID_BEGIN_HEADER

// GOLDFISH SYNC DEVICE
// The Goldfish sync driver is designed to provide a interface
// between the underlying host's sync device and the kernel's
// sw_sync.
// The purpose of the device/driver is to enable lightweight
// creation and signaling of timelines and fences
// in order to synchronize the guest with host-side graphics events.

// Each time the interrupt trips, the driver
// may perform a sw_sync operation.

// The operations are:

// Ready signal - used to mark when irq should lower
#define CMD_SYNC_READY            0

// Create a new timeline. writes timeline handle
#define CMD_CREATE_SYNC_TIMELINE  1

// Create a fence object. reads timeline handle and time argument.
// Writes fence fd to the SYNC_REG_HANDLE register.
#define CMD_CREATE_SYNC_FENCE     2

// Increments timeline. reads timeline handle and time argument
#define CMD_SYNC_TIMELINE_INC     3

// Destroys a timeline. reads timeline handle
#define CMD_DESTROY_SYNC_TIMELINE 4

// Starts a wait on the host with
// the given glsync object and sync thread handle.
#define CMD_TRIGGER_HOST_WAIT     5

// The register layout is:

#define SYNC_REG_BATCH_COMMAND                0x00 // host->guest batch commands
#define SYNC_REG_BATCH_GUESTCOMMAND           0x04 // guest->host batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR           0x08 // communicate physical address of host->guest batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR_HIGH      0x0c // 64-bit part
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR      0x10 // communicate physical address of guest->host commands
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR_HIGH 0x14 // 64-bit part

#define GUEST_TRIGGERED(cmd) (cmd == CMD_SYNC_READY || \
                              cmd == CMD_TRIGGER_HOST_WAIT)

typedef void (*trigger_wait_fn_t)(uint64_t, uint64_t, uint64_t);

// |goldfish_sync_create_timeline| sets up state for synchronizing
// events. The timeline is represented as a monotonically increasing
// counter. Fence objects are represented by one or more timeline numbers
// corresponding to these abstract times.
// When the timeline advances past a particular number, and the fence
// object itself has had all its timeline points <= all corresponding
// timeline counters, the fence object is considered signaled.
// This and all other functions trigger similarly-named versions
// in the guest kernel with |sw_sync_| or |sync_| as prefix.
uint64_t goldfish_sync_create_timeline();

// |goldfish_sync_create_fence| associates a fence object with a single
// timeline point |pt| for the |timeline|. After |timeline| advances
// up to |pt|, the fence object is considered signaled. This function
// will also return a file descriptor number from the guest representing
// the fence object, allowing us to pass it around.
// Since we do not allow fences to be associated with multiple points or
// timelines, for our purposes, every fence can be considered just
// tied to one timeline point. We would have to connect to
// |sync_fence_merge| in the guest kernel for that
// (which would create a fence associated with the union of points
// of two input fence objects).
int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);

// |goldfish_sync_timeline_inc| advances |timeline|'s counter by |howmuch|.
// Any fence objects whose point is <= howmuch get signaled.
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);

// |goldfish_sync_destroy_timeline| cleans up the timeline object
// by calling a similarly named |sw_sync| function in the guest kernel.
// Any fence objects whose point is <= howmuch get signaled.
void goldfish_sync_destroy_timeline(uint64_t timeline);

// Registering callbacks for goldfish sync ops
// Currently, only trigger_wait is supported.
void goldfish_sync_register_trigger_wait(
        trigger_wait_fn_t trigger_fn);

// Function types for sending commands to the virtual device.
typedef void (*queue_device_command_t)
    (uint32_t cmd, uint64_t handle, uint32_t time_arg,
     uint64_t hostcmd_handle);

typedef struct GoldfishSyncDeviceInterface {
    // Callback for all communications with virtual device
    queue_device_command_t doHostCommand;

    // Callbacks to register other callbacks for triggering
    // OpenGL waits from the guest
    void (*registerTriggerWait)(trigger_wait_fn_t);
} GoldfishSyncDeviceInterface;

// The virtual device will call |goldfish_sync_set_hw_funcs|
// to connect up to the AndroidEmu part.
extern void
goldfish_sync_set_hw_funcs(GoldfishSyncDeviceInterface* hw_funcs);

// The virtual device calls this |goldfish_sync_receive_hostcmd_result|
// when it receives a reply for host->guest commands that support
// that protocol.
extern void
goldfish_sync_receive_hostcmd_result(uint32_t cmd,
                                     uint64_t handle,
                                     uint32_t time_arg,
                                     uint64_t hostcmd_handle);
ANDROID_END_HEADER
