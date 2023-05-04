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

#pragma once

#include "aemu/base/Compiler.h"
#include "aemu/base/Optional.h"

#include "android/base/system/System.h"
#include "snapshot/common.h"
#include "android/snapshot/RamSaver.h"
#include "android/snapshot/Snapshot.h"

#include <string_view>

namespace android {
namespace snapshot {

class RamLoader;

class Saver {
    DISALLOW_COPY_AND_ASSIGN(Saver);

public:
    Saver(const Snapshot& snapshot,
          RamLoader* loader,
          bool isOnExit,
          std::string_view ramMapFile,
          bool ramFileShared,
          bool isRemapping);
    ~Saver();

    RamSaver& ramSaver() { return *mRamSaver; }
    ITextureSaverPtr textureSaver() const;

    OperationStatus status() const { return mStatus; }
    const Snapshot& snapshot() const { return mSnapshot; }

    void prepare();
    void complete(bool succeeded);

    bool incrementallySaved() const { return mIncrementallySaved; }

    void cancel();

    bool canceled() const { return mStatus == OperationStatus::Canceled; }

    const base::MemUsage& memUsage() const { return mMemUsage; }
    bool isHDD() const { return mDiskKind.valueOr(base::DiskKind::Ssd) ==
                                    base::DiskKind::Hdd; }

private:
    OperationStatus mStatus;
    Snapshot mSnapshot;
    base::Optional<RamSaver> mRamSaver;
    std::shared_ptr<TextureSaver> mTextureSaver;
    bool mIncrementallySaved = false;
    base::MemUsage mMemUsage;
    base::Optional<base::DiskKind> mDiskKind = {};
};

}  // namespace snapshot
}  // namespace android
