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

    if (!mSnapshot.preload()) {
        return;
    }
    {
        const auto ram = fopen(
                PathUtils::join(mSnapshot.dataDir(), "ram.bin").c_str(), "rb");
        if (!ram) {
            mSnapshot.saveFailure(FailureReason::NoRamFile);
            return;
        }
        mRamLoader.emplace(StdioStream(ram, StdioStream::kOwner));
    }
    {
        const auto textures = fopen(
                PathUtils::join(mSnapshot.dataDir(), "textures.bin").c_str(),
                "rb");
        if (!textures) {
            mSnapshot.saveFailure(FailureReason::NoTexturesFile);
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

void Loader::start() {
    if (!mSnapshot.load()) {
        mStatus = OperationStatus::Error;
    }
}

Loader::~Loader() {
    // Wait for textureLoader to finish loading textures
    if (mRamLoader && !mRamLoader->hasError()) {
        mRamLoader->join();
    }
    if (mTextureLoader) {
        mTextureLoader->join();
    }
}

void Loader::complete(bool succeeded) {
    mStatus = OperationStatus::Error;
    if (!succeeded) {
        if (!mSnapshot.failureReason()) {
            mSnapshot.saveFailure(FailureReason::EmulationEngineFailed);
        }
        return;
    }
    if (!mRamLoader || mRamLoader->hasError()) {
        if (!mSnapshot.failureReason()) {
            mSnapshot.saveFailure(FailureReason::RamFailed);
        }
        return;
    }
    if (!mTextureLoader || mTextureLoader->hasError()) {
        if (!mSnapshot.failureReason()) {
            mSnapshot.saveFailure(FailureReason::TexturesFailed);
        }
        return;
    }

    mStatus = OperationStatus::Ok;
}

}  // namespace snapshot
}  // namespace android
