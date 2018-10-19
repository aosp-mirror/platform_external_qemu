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

#include "android/base/files/FileShareOpen.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "android/snapshot/common.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

using android::base::c_str;
using android::base::PathUtils;
using android::base::StdioStream;
using android::base::StringView;
using android::base::System;

namespace android {
namespace snapshot {

Saver::Saver(const Snapshot& snapshot, RamLoader* loader, bool isOnExit,
             base::StringView ramMapFile, bool ramFileShared, bool isRemapping)
    : mStatus(OperationStatus::Error), mSnapshot(snapshot) {
    if (path_mkdir_if_needed_no_cow(c_str(mSnapshot.dataDir()), 0777) != 0) {
        return;
    }
    {
        const auto ramFile = PathUtils::join(mSnapshot.dataDir(), kRamFileName);
        auto flags = RamSaver::Flags::None;

        // If we're using a file-backed RAM that is writing through,
        // no need to save pages synchronously on exit.
        if ((!ramMapFile.empty() &&
             ramFileShared) &&
            (isOnExit || isRemapping)) {
            flags |= RamSaver::Flags::Async;
        }

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
            auto numCores = System::get()->getCpuCoreCount();
            mMemUsage = System::get()->getMemUsage();
            mDiskKind =
                    System::get()->pathDiskKind(mSnapshot.dataDir());
            if (numCores > 2) {
                auto freeMb = mMemUsage.avail_phys_memory / (1024 * 1024);
                if (freeMb < 1536) {
                    flags |= RamSaver::Flags::Compress;
                    VERBOSE_PRINT(
                            snapshot,
                            "Enabling RAM compression: only %u MB of free RAM",
                            unsigned(freeMb));
                } else {
                    if (mDiskKind.valueOr(System::DiskKind::Ssd) ==
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
        const auto textures = android::base::fsopen(
                PathUtils::join(mSnapshot.dataDir(), kTexturesFileName).c_str(),
                "wb", android::base::FileShare::Write);
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
        path_delete_dir(c_str(mSnapshot.dataDir()));
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

    base::System::Duration ramDuration = 0;
    base::System::Duration texturesDuration = 0;

    if (mRamSaver->getDuration(&ramDuration) &&
        mTextureSaver->getDuration(&texturesDuration)) {

        mSnapshot.addSaveStats(
                mIncrementallySaved,
                ramDuration + texturesDuration,
                0 /* ram changed bytes; unused for now */);

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
    path_delete_dir(c_str(mSnapshot.dataDir()));

    mSnapshot.saveFailure(FailureReason::Canceled);
}

}  // namespace snapshot
}  // namespace android
