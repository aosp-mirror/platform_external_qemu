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

// The commands below operate on Android sync "timelines" and
// "fences". The basic concept is as follows.
// - Timeline and fences work together as a state to determine
//   which events have completed. The timeline abstraction
//   is used to represent a monotonic ordering of events,
//   while fences represent levels of progress along
//   timelines. By having different threads wait on fences
//   until their associated timelines make the required progress,
//   we can synchronize multiple threads within a process
//   and with binder, across processes.
// - What follows is an abstract definition of that concept
//   that does not match implementation exactly, but is
//   sufficient for our purposes:
// - A timeline is a mutable pair (uint64_t, uint32_t) of
//   (timeline_id, value). Over time, the value component
//   may increase.
// - A fence is a uniquely identified immutable set
//   (int, { (uint64_t, uint32_t) })
//   of pairs of timeline-id-values. Over time, the set
//   nor the items within may change. The first component is
//   often referred to as the fence fd.
// - The important state is represented by a collection of
//   timelines and fences ( Ts = {T_i}, Fs = {F_j} )
//   (a pair consisting of a set of timelines and a set of fences)
//   can have the following properties:
//   - The set of timelines has unique ids: i.e., the t in each
//     (t, v) in Ts only occurs once in Ts.
//   - Fences only name one value in their corresponding timelines.
//     For each f = (fence_id, timeline-id-values) in Fs,
//     and for each (t, v) in timeline-id-values,
//     the t only occurs once in timeline-id-values
//     (but it's OK for the same t to show up
//     in different f, g in Fs).
//   - Each fence (f in Fs) is either unsignaled or signaled
//     (modulo errors):
//     - Signaled: For all timeline-id-values (t, v) in f,
//       there is a (t, w) in Ts (timeline set) such that
//       w >= v. Or, there is no timeline id t in Ts.
//     - Unsignaled: Not signaled.
// The functions below operate with the above abstraction in mind.
// They all mutate a particular collection of fences and timelines.
// We can think of them as implicitly taking a collection (Ts, Fs)
// as input and returning a new one (Ts', Fs') that is possibly updated,
// according to the changes described. The new collection then
// serves as the true abstract state for all further operations.

// |goldfish_sync_create_timeline| creates a new timeline
// with value 0. The timeline id is returned.
uint64_t goldfish_sync_create_timeline();

// |goldfish_sync_create_fence| creates a fence with unique id
// that has a single timeline-id-value pair. The first two
// arguments comprise the pair, and the fence id is returned.
// Note that this implementation only allows the creation of
// fences associated with a single timeline-id-value pair.
int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);

// |goldfish_sync_timeline_inc| advances the value component
// of a given timeline by |howmuch|; that is, if
// (t, v) is the timeline id and value of a timeline before this call,
// (t, v + |howmuch|) is the value after.
// Thus, this may end up changing some fence objects to signaled state.
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);

// |goldfish_sync_destroy_timeline| removes the timeline (t, v)
// where t == |timeline| from the set of timelines.
// Any fence objects with timeline-id-values (t, v) where
// t == |timeline| would then be considered signaled.
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
