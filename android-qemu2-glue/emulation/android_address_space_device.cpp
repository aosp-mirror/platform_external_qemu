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

#include "android-qemu2-glue/emulation/android_address_space_device.h"
#include "android-qemu2-glue/base/files/QemuFileStream.h"
#include "android/emulation/address_space_device.hpp"
#include "android/utils/stream.h"

extern "C" {
#include "hw/pci/goldfish_address_space.h"
}  // extern "C"

using android::qemu::QemuFileStream;
using android::emulation::goldfish_address_space_memory_state_load;
using android::emulation::goldfish_address_space_memory_state_save;

namespace {
const GoldfishAddressSpaceOps ops = {
    // .load
    [](QEMUFile* file) -> int {
        QemuFileStream stream(file);
        return goldfish_address_space_memory_state_load(&stream);
    },
    // .save
    [](QEMUFile* file) -> int {
        QemuFileStream stream(file);
        return goldfish_address_space_memory_state_save(&stream);
    }
};
}

bool qemu_android_address_space_device_init() {
    goldfish_address_space_set_service_ops(&ops);
    return true;
}
