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

// Ready signal
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

// The register layout is:

#define SYNC_REG_COMMAND      0x00 // which operation to run
#define SYNC_REG_HANDLE       0x04 // Timeline or fence fd to operator on
#define SYNC_REG_HANDLE_HIGH  0x08 // 64-bit part
#define SYNC_REG_TIME_ARG     0x0c // how much to increment timeline

#define NO_WRITE_BACK(cmd) (cmd == CMD_SYNC_TIMELINE_INC || \
                            cmd == CMD_DESTROY_SYNC_TIMELINE)

uint64_t goldfish_sync_create_timeline();
int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);
void goldfish_sync_destroy_timeline(uint64_t timeline);

typedef struct GoldfishSyncHwFuncs {
    uint64_t (*createTimeline)();
    int (*createFence)(uint64_t timeline, uint32_t pt);
    void (*incTimeline)(uint64_t timeline, uint32_t howmuch);
    void (*destroyTimeline)(uint64_t timeline);
} GoldfishSyncHwFuncs;

extern const GoldfishSyncHwFuncs*
goldfish_sync_set_hw_funcs(const GoldfishSyncHwFuncs* hw_funcs);

ANDROID_END_HEADER
