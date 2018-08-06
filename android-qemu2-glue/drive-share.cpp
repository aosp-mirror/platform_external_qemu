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
#include <vector>

#include "android/base/memory/LazyInstance.h"
#include "android/base/files/FileShareOpen.h"
#include "android/multi-instance.h"

extern "C" {
#include "qemu/osdep.h"
#include "block/block.h"
#include "block/block_int.h"
#include "sysemu/blockdev.h"
}

namespace {
    struct DriveShare {
        std::vector<DriveInfo*> driveInfoList = {};
        std::vector<QemuOpts*> qemuOptsList = {};
        std::vector<BlockInterfaceType> blockDefaultTypeList = {};
    };
    static android::base::LazyInstance<DriveShare> sDriveShare =
            LAZY_INSTANCE_INIT;
}

static void updateDriveShareMode(android::base::FileShare shareMode) {
    return;
    //for (auto& driveInfo : sDriveShare->driveInfoList) {
    //    drive_uninit(driveInfo);
    //}
    blockdev_close_all_bdrv_states();
    sDriveShare->driveInfoList.clear();
    for (size_t i = 0; i < sDriveShare->qemuOptsList.size(); i++) {
        DriveInfo* driveInfo = drive_new(sDriveShare->qemuOptsList[i],
                sDriveShare->blockDefaultTypeList[i]);
        assert(driveInfo);
        sDriveShare->driveInfoList.push_back(driveInfo);
    }
}

extern "C" void android_drive_share_init() {
    android::multiinstance::setUpdateDriveShareModeFunc(updateDriveShareMode);
}

extern "C" int android_drive_init(void* opaque, QemuOpts* opts, Error** errp) {
    BlockInterfaceType* block_default_type = (BlockInterfaceType*)opaque;
    DriveInfo* driveInfo = drive_new(opts, *block_default_type);
    if (driveInfo) {
        sDriveShare->driveInfoList.push_back(driveInfo);
        sDriveShare->qemuOptsList.push_back(opts);
        if (block_default_type) {
            sDriveShare->blockDefaultTypeList.push_back(*block_default_type);
        } else {
            sDriveShare->blockDefaultTypeList.push_back(IF_NONE);
        }
        return 0;
    } else {
        return 1;
    }
}
