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

#include "android/emulation/goldfish_hostmem.h"
#include "android/emulation/GoldfishHostmemCommandQueue.h"

using android::GoldfishHostmemCommandQueue;

static GoldfishHostmemDeviceInterface* sGoldfishHostmemHwFuncs = NULL;

// Goldfish hostmem host-side interface implementation/////////////////////////////

static void sendCommand(uint64_t gpa, uint64_t id, void* host_ptr, uint64_t size, uint64_t status_code, uint64_t cmd) {
    GoldfishHostmemCommandQueue::hostSignal
        (gpa, id, host_ptr, size, status_code, cmd);
}

void goldfish_hostmem_set_ptr(uint64_t id, void* host_ptr, uint64_t size, uint64_t status_code) {
    sendCommand(0 /* TODO */, id, host_ptr, size, status_code, CMD_HOSTMEM_SET_PTR);
}

void goldfish_hostmem_unset_ptr(uint64_t id, uint64_t status_code) {
    sendCommand(0, id, 0, 0, status_code, CMD_HOSTMEM_UNSET_PTR);
}

bool goldfish_hostmem_device_exists() {
    // The idea here is that the virtual device should set
    // sGoldfishHostmemHwFuncs. If it didn't do that, we take
    // that to mean there is no virtual device.
    return sGoldfishHostmemHwFuncs != NULL;
}

void goldfish_hostmem_set_hw_funcs(GoldfishHostmemDeviceInterface* hw_funcs) {
    sGoldfishHostmemHwFuncs = hw_funcs;
    GoldfishHostmemCommandQueue::setQueueCommand
        (sGoldfishHostmemHwFuncs->doHostCommand);
}
