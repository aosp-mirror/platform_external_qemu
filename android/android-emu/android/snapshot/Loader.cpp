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

#include "android/snapshot/Loader.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/snapshot/TextureLoader.h"
#include "android/utils/path.h"

using android::base::PathUtils;
using android::base::StdioStream;

namespace android {
namespace snapshot {

Loader::Loader(const Snapshot& snapshot)
    : mStatus(OperationStatus::Error), mSnapshot(snapshot) {
    if (!path_is_dir(mSnapshot.dataDir().c_str())) {
        return;
    }

    if (!mSnapshot.load()) {
        return;
    }
    {
        const auto ram = fopen(
                PathUtils::join(mSnapshot.dataDir(), "ram.bin").c_str(), "rb");
        if (!ram) {
            return;
        }
        mRamLoader.emplace(StdioStream(ram, StdioStream::kOwner));
    }
    {
        const auto textures = fopen(
                PathUtils::join(mSnapshot.dataDir(), "textures.bin").c_str(),
                "rb");
        if (!textures) {
            mRamLoader.clear();
            return;
        }
        mTextureLoader = std::make_shared<TextureLoader>(
                StdioStream(textures, StdioStream::kOwner));
    }

    mStatus = OperationStatus::NotStarted;
}

void Loader::prepare() {
    // TODO: run asynchronous index loading here.
}

Loader::~Loader() = default;

void Loader::complete(bool succeeded) {
    mStatus = OperationStatus::Error;
    if (!succeeded) {
        return;
    }
    if (!mRamLoader || mRamLoader->hasError()) {
        return;
    }
    if (!mTextureLoader || mTextureLoader->hasError()) {
        return;
    }

    mStatus = OperationStatus::Ok;
}

}  // namespace snapshot
}  // namespace android
