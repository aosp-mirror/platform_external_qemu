// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/drive-share.h"

#include <cstdint>
#include <cstdio>

#include "android/base/files/FileShareOpen.h"
#include "android/multi-instance.h"

extern "C" {
#include "qemu/osdep.h"
#include "block/block.h"
#include "sysemu/blockdev.h"
}

static void updateDriveShareMode(android::base::FileShare shareMode) {}

extern "C" void android_drive_share_init() {
    android::multiinstance::setUpdateDriveShareModeFunc(updateDriveShareMode);
}

extern "C" int android_drive_init(void* opaque, QemuOpts* opts, Error** errp) {
    BlockInterfaceType* block_default_type = (BlockInterfaceType*)opaque;

    return drive_new(opts, *block_default_type) == NULL;
}
