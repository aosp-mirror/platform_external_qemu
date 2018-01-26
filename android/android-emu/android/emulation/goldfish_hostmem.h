/* Copyright (C) 2018 The Android Open Source Project
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
#include <stdbool.h>

ANDROID_BEGIN_HEADER

// GOLDFISH HOSTMEM DEVICE
// The Goldfish hostmem driver is designed to map memory from the host
// into the guest.
// The purpose of the device/driver is to enable correct and lightweight
// mapped buffers in OpenGL / Vulkan.

// Each time the interrupt trips, the guest kernel is notified of changes in
// mapped host memory regions. They will occur in GPAs outside the normal
// physical RAM space of the guest.

// The operations are:

#define CMD_HOSTMEM_READY            0
#define CMD_HOSTMEM_SET_PTR          1
#define CMD_HOSTMEM_UNSET_PTR        2

// The register layout is:

#define HOSTMEM_REG_BATCH_COMMAND                0x00 // host->guest batch commands
#define HOSTMEM_REG_BATCH_COMMAND_ADDR           0x08 // communicate physical address of host->guest batch commands
#define HOSTMEM_REG_BATCH_COMMAND_ADDR_HIGH      0x0c // 64-bit part
#define HOSTMEM_REG_INIT                         0x18 // to signal that the device has been detected by the kernel

// If the virtual device doesn't actually exist (e.g., when
// using QEMU1), query that using this function:
bool goldfish_hostmem_device_exists();

void goldfish_hostmem_set_ptr(uint64_t id, void* host_ptr, uint64_t size, uint64_t status_code);
void goldfish_hostmem_unset_ptr(uint64_t id, uint64_t status_code);

// Function types for sending commands to the virtual device.
typedef void (*hostmem_queue_device_command_t)
    (uint64_t gpa, uint64_t id, void* host_ptr, uint64_t size, uint64_t status_code, uint64_t cmd);

typedef struct GoldfishHostmemDeviceInterface {
    // Callback for all communications with virtual device
    hostmem_queue_device_command_t doHostCommand;
} GoldfishHostmemDeviceInterface;

// The virtual device will call |goldfish_hostmem_set_hw_funcs|
// to connect up to the AndroidEmu part.
extern void
goldfish_hostmem_set_hw_funcs(GoldfishHostmemDeviceInterface* hw_funcs);

ANDROID_END_HEADER

