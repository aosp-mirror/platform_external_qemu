/* Copyright 2018 The Android Open Source Project
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
#include "android-qemu2-glue/emulation/goldfish_hostmem.h"

// Glue code between the virtual goldfish hostmem device, and the
// host-side hostmem service implementation.

#include "android/base/Log.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/GoldfishHostmemCommandQueue.h"
#include "android/emulation/goldfish_hostmem.h"
#include "android/utils/stream.h"
#include "android-qemu2-glue/base/files/QemuFileStream.h"

using android::qemu::QemuFileStream;

extern "C" {
#include "hw/misc/goldfish_hostmem.h"
}  // extern "C"

// These callbacks are called from the host hostmem service to operate
// on the virtual device.
static GoldfishHostmemDeviceInterface kHostmemDeviceInterface = {
    .doHostCommand = goldfish_hostmem_send_command,
};

static const GoldfishHostmemServiceOps kHostmemServiceOps = {
    .save = [](QEMUFile* file) {
        QemuFileStream stream(file);
        android::GoldfishHostmemCommandQueue::save(&stream);
    },
    .load = [](QEMUFile* file) {
        QemuFileStream stream(file);
        android::GoldfishHostmemCommandQueue::load(&stream);
    },
};

bool qemu_android_hostmem_init(android::VmLock* vmLock) {
    goldfish_hostmem_set_hw_funcs(&kHostmemDeviceInterface);
    goldfish_hostmem_set_service_ops(&kHostmemServiceOps);
    android::GoldfishHostmemCommandQueue::initThreading(vmLock);
    return true;
}
