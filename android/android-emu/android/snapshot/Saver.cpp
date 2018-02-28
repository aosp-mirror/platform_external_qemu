// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Saver.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/system/System.h"
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

using android::base::PathUtils;
using android::base::StdioStream;
using android::base::System;

namespace android {
namespace snapshot {

Saver::Saver(const Snapshot& snapshot, RamLoader* loader, bool isOnExit)
    : mStatus(OperationStatus::Error), mSnapshot(snapshot) {
    if (path_mkdir_if_needed(mSnapshot.dataDir().c_str(), 0777) != 0) {
        return;
    }
    {
        const auto ramFile = PathUtils::join(mSnapshot.dataDir(), "ram.bin");
        auto flags = RamSaver::Flags::None;
        const auto compressEnvVar =
                System::get()->envGet("ANDROID_SNAPSHOT_COMPRESS");
        if (compressEnvVar == "1" || compressEnvVar == "yes" ||
            compressEnvVar == "true") {
            VERBOSE_PRINT(snapshot,
                          "autoconfig: enabled snapshot RAM compression from "
                          "environment [ANDROID_SNAPSHOT_COMPRESS=%s]",
                          compressEnvVar.c_str());
            flags |= RamSaver::Flags::Compress;
        } else if (compressEnvVar == "0" || compressEnvVar == "no" ||
                   compressEnvVar == "false") {
            // don't enable compression
            VERBOSE_PRINT(snapshot,
                          "autoconfig: forced no snapshot RAM compression from "
                          "environment [ANDROID_SNAPSHOT_COMPRESS=%s]",
                          compressEnvVar.c_str());
        } else {
            // Check if it's faster to save RAM with compression. Currently
            // the heuristics are as following:
            // 1. Gather three variables: free RAM, number of CPU cores and
            //    the disk kind.
            // 2. Bad cases for uncompressed snapshots: little free RAM
            //    and HDD disk.
            // 3. If there's a bad case and we have enough CPU cores (3+), let's
            //    enable compression.
            const auto cpuCount = System::get()->getCpuCoreCount();
            if (cpuCount > 2) {
                const auto memUsage = System::get()->getMemUsage();
                if (memUsage.avail_phys_memory < 1536 * 1024 * 1024) {
                    flags |= RamSaver::Flags::Compress;
                    VERBOSE_PRINT(
                            snapshot,
                            "Enabling RAM compression: only %u MB of free RAM",
                            unsigned(memUsage.avail_phys_memory /
                                     (1024 * 1024)));
                } else {
                    // Disk kind calculation is potentially the slowest: do it
                    // last.
                    const auto diskKind =
                            System::get()->pathDiskKind(mSnapshot.dataDir());
                    if (diskKind.valueOr(System::DiskKind::Ssd) ==
                        System::DiskKind::Hdd) {
                        flags |= RamSaver::Flags::Compress;
                        VERBOSE_PRINT(
                                snapshot,
                                "Enabling RAM compression: snapshot is on HDD");
                    }
                }
            }
        }

        const bool tryIncremental =
            loader && !loader->hasError() && loader->hasGaps();

        mIncrementallySaved = tryIncremental;

        mRamSaver.emplace(ramFile, flags, tryIncremental ? loader : nullptr,
                          isOnExit);
        if (mRamSaver->hasError()) {
            mRamSaver.clear();
            return;
        }
    }

    {
        const auto textures = fopen(
                PathUtils::join(mSnapshot.dataDir(), "textures.bin").c_str(),
                "wb");
        if (!textures) {
            mRamSaver.clear();
            return;
        }
        mTextureSaver = std::make_shared<TextureSaver>(
                StdioStream(textures, StdioStream::kOwner));
    }

    mStatus = OperationStatus::NotStarted;
}

Saver::~Saver() {
    const bool deleteDirectory =
            mStatus != OperationStatus::Ok && (mRamSaver || mTextureSaver);
    mRamSaver.clear();
    mTextureSaver.reset();
    if (deleteDirectory) {
        path_delete_dir(mSnapshot.dataDir().c_str());
    }
}

ITextureSaverPtr Saver::textureSaver() const {
    return mTextureSaver;
}

void Saver::prepare() {
    // TODO: run asynchronous saving preparation here (e.g. screenshot,
    // hardware info collection etc).
}

void Saver::complete(bool succeeded) {
    mStatus = OperationStatus::Error;
    if (!succeeded) {
        return;
    }
    if (!mRamSaver || mRamSaver->hasError()) {
        return;
    }
    mRamSaver->join();
    if (!mTextureSaver ||
        (static_cast<void>(mTextureSaver->done()), mTextureSaver->hasError())) {
        return;
    }

    if (!mSnapshot.save()) {
        return;
    }

    mStatus = OperationStatus::Ok;
}

void Saver::cancel() {
    mStatus = OperationStatus::Canceled;

    if (mRamSaver) {
        mRamSaver->cancel();
    }

    // TODO next: texture save cancel
    path_delete_dir(mSnapshot.dataDir().c_str());

    mSnapshot.saveFailure(FailureReason::Canceled);
}

}  // namespace snapshot
}  // namespace android
