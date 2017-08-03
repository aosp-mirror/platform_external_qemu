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
#include "android/utils/path.h"
#include "android/snapshot/TextureSaver.h"

#include <fstream>

using android::base::PathUtils;
using android::base::StdioStream;

namespace android {
namespace snapshot {

Saver::Saver(const Snapshot& snapshot)
    : mStatus(OperationStatus::Error), mSnapshot(snapshot) {
    if (path_mkdir_if_needed(mSnapshot.dataDir().c_str(), 0777) != 0) {
        return;
    }
    {
        const auto ram = fopen(
                PathUtils::join(mSnapshot.dataDir(), "ram.bin").c_str(), "wb");
        if (!ram) {
            return;
        }
        mRamSaver.emplace(StdioStream(ram, StdioStream::kOwner),
                          RamSaver::Flags::None);
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
    const bool deleteDirectory = mStatus != OperationStatus::Ok &&
                                 (mRamSaver || mTextureSaver);
    mRamSaver.clear();
    mTextureSaver.reset();
    if (deleteDirectory) {
        path_delete_dir(mSnapshot.dataDir().c_str());
    }
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
    if (!mTextureSaver ||
        (static_cast<void>(mTextureSaver->done()), mTextureSaver->hasError())) {
        return;
    }

    if (!mSnapshot.save()) {
        return;
    }

    mStatus = OperationStatus::Ok;
}

}  // namespace snapshot
}  // namespace android
