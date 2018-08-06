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
#include "sysemu/block-backend.h"
#include "block/block.h"
#include "block/block_int.h"
#include "sysemu/blockdev.h"
#include "sysemu/sysemu.h"
#include "qapi/qmp/qstring.h"
#include "qapi/error.h"
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

static void printBdrv(BlockDriverState* bs) {
    fprintf(stderr, "bs %p refcnt %d file %s backing file %s\n",
            bs, bs->refcnt, bs->filename, bs->backing_file);
}

static void updateDriveShareMode(android::base::FileShare shareMode) {
    //for (auto& driveInfo : sDriveShare->driveInfoList) {
    //    drive_uninit(driveInfo);
    //}
    BdrvNextIterator it;
    BlockDriverState* bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bs = bdrv_next(&it);
    }
    std::vector<BlockBackend*> blkList;
    blkList.reserve(sDriveShare->driveInfoList.size());
    for (const auto& driveInfo : sDriveShare->driveInfoList) {
        for (BlockBackend* blk = blk_next(NULL); blk; blk = blk_next(blk)) {
            if (blk_legacy_dinfo(blk) == driveInfo) {
                blkList.push_back(blk);
                break;
            }
        }
    }
    assert(blkList.size() == sDriveShare->driveInfoList.size());
    printf("close all bdrv\n");
    //blockdev_close_all_bdrv_states();
    /*bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bdrv_unref(bs);
        bs = bdrv_first(&it);
    }*/
    bdrv_close_all();
    printf("close all bdrv done\n");
    bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bs = bdrv_next(&it);
    }
    sDriveShare->driveInfoList.clear();
    for (size_t i = 0; i < sDriveShare->qemuOptsList.size(); i++) {
        QemuOpts* opts = sDriveShare->qemuOptsList[i];
        QDict *bs_opts = qdict_new();
        qemu_opts_to_qdict(opts, bs_opts);
        const char* file = qemu_opt_get(opts, "file");
        QemuOpts* legacy_opts = qemu_opts_create(&qemu_legacy_drive_opts, NULL, 0,
                                   nullptr);
        qemu_opts_absorb_qdict(legacy_opts, bs_opts, nullptr);

        bool read_only = qemu_opt_get_bool(legacy_opts, BDRV_OPT_READ_ONLY, false);
        printf("read only %d\n", read_only);
        printf("id %s\n", qdict_get_try_str(bs_opts, "id"));
        qdict_put(bs_opts, BDRV_OPT_READ_ONLY,
              qstring_from_str(read_only ? "on" : "off"));
        qdict_put(bs_opts, "cache.no-flush", qstring_from_str("off"));
        qdict_put(bs_opts, "cache.direct", qstring_from_str("off"));
        qdict_del(bs_opts, "id");

        printf("creating new drive %s\n", file);
        assert(file && file[0]);
        Error *local_err = NULL;
        //file = "/usr/local/google/home/yahan/Android/Sdk/system-images/android-P/google_apis_playstore/x86_64//system.img";
        printf("filename: %s\n", file);
        qdict_print(bs_opts);
        BlockDriverState *bs = bdrv_open(file, nullptr, bs_opts, 0, &local_err);
        if (local_err) {
            error_report_err(local_err);
            assert(false);
        }

        BlockBackend* blk = blkList[i];
        assert(blk);
        assert(bs);
        // QEMU does not really delete blk_legacy_dinfo
        //assert(!blk_legacy_dinfo(blk));
        int res = blk_insert_bs(blk, bs, nullptr);
        assert(res == 0);

        DriveInfo* driveInfo = drive_new(sDriveShare->qemuOptsList[i],
                sDriveShare->blockDefaultTypeList[i]);
        assert(driveInfo);
        sDriveShare->driveInfoList.push_back(driveInfo);
    }
    bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bs = bdrv_next(&it);
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
