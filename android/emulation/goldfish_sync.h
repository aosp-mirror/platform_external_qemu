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

uint64_t goldfish_sync_create_timeline();
int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);
void goldfish_sync_destroy_timeline(uint64_t timeline);

// Registering callbacks for goldfish sync ops
// Currently, only trigger_wait is supported.
void goldfish_sync_register_trigger_wait(
        trigger_wait_fn_t trigger_fn);

typedef struct GoldfishSyncHwFuncs2 {
    void (*goldfishSyncSendCmd)(void* cmd);
    void (*goldfishSyncRecvCmd)(void* cmd);
} GoldfishSyncHwFuncs2;

// Function types for sending:
typedef void (*queue_device_command_t)
    (uint32_t cmd, uint64_t handle, uint32_t time_arg,
     uint64_t hostcmd_handle);

// and receiving:
typedef void (*device_command_result_t)
    (void* data,
     uint32_t cmd, uint64_t handle, uint32_t time_arg,
     uint64_t hostcmd_handle);
// commands from the virtual device.

typedef struct GoldfishSyncDeviceInterface {
    // Callback for all communications with virtual device
    queue_device_command_t doHostCommand;
    device_command_result_t receiveCommandResult;

    // Callbacks to register other callbacks
    void (*registerTriggerWait)(trigger_wait_fn_t);

    // Pointer to command queue
    void* context;
} GoldfishSyncDeviceInterface;

extern void
goldfish_sync_set_hw_funcs(GoldfishSyncDeviceInterface* hw_funcs);

extern void
goldfish_sync_set_cmd_receiver(GoldfishSyncDeviceInterface* hw_funcs,
                               device_command_result_t next_recv);

ANDROID_END_HEADER
