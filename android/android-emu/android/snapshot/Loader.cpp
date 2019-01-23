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
#include <errno.h>

#include "android/base/files/FileShareOpen.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/snapshot/TextureLoader.h"
#include "android/utils/path.h"
#include "android/utils/file_io.h"

using android::base::PathUtils;
using android::base::StdioStream;
using android::base::System;

namespace android {
namespace snapshot {

Loader::Loader(const Snapshot& snapshot, int error)
    : mStatus(OperationStatus::Error), mSnapshot(snapshot) {
    if (error) {
        mSnapshot.saveFailure(errnoToFailure(error));
        return;
    }

    if (!path_is_dir(base::c_str(mSnapshot.dataDir()))) {
        return;
    }

    if (!mSnapshot.preload()) {
        return;
    }

    mMemUsage = System::get()->getMemUsage();
    mDiskKind = System::get()->pathDiskKind(mSnapshot.dataDir());

    {
        const auto ram = android::base::fsopen(
                PathUtils::join(mSnapshot.dataDir(), kRamFileName).c_str(),
                "rb", android::base::FileShare::Read);
        if (!ram) {
            mSnapshot.saveFailure(FailureReason::NoRamFile);
            return;
        }

        // TODO: RamLoader constructor can just have the first argument if we
        // can figure out a way to get it to compile for windows. It currently
        // doesn't like {} being put as an argument for the ram block structure
        // directly.

        RamLoader::RamBlockStructure emptyRamBlockStructure = {};
        mRamLoader.emplace(StdioStream(ram, StdioStream::kOwner),
                           RamLoader::Flags::OnDemandAllowed,
                           emptyRamBlockStructure);
    }
    {
        const auto textures = android::base::fsopen(
                PathUtils::join(mSnapshot.dataDir(), "textures.bin").c_str(),
                "rb", android::base::FileShare::Read);
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

void Loader::reportSuccessful() {
    mSnapshot.incrementSuccessfulLoads();
}

void Loader::reportInvalid() {
    mSnapshot.incrementInvalidLoads();
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

void Loader::interrupt() {
    if (mRamLoader && !mRamLoader->hasError()) {
        mRamLoader->interrupt();
    }

    if (mTextureLoader && !mTextureLoader->hasError()) {
        mTextureLoader->interrupt();
    }
}

ITextureLoaderPtr Loader::textureLoader() const {
    return mTextureLoader;
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

// Don't do heavy operations like interrupting the loader
// here since this could be in a crash handler.
void Loader::onInvalidSnapshotLoad() {
    mSnapshot.incrementInvalidLoads();
    if (mSnapshot.shouldInvalidate()) {
        mSnapshot.saveFailure(FailureReason::CorruptedData);
    } else {
        mSnapshot.saveFailure(FailureReason::InternalError);
    }
}

void Loader::join() {
    if (mRamLoader) {
        mRamLoader->touchAllPages();
    }
    if (mTextureLoader) {
        mTextureLoader->join();
    }
}

void Loader::synchronize(bool isOnExit) {
    if (mTextureLoader) {
        mTextureLoader->join();
    }

    // Prepare ram loader for incremental save.
    //
    // It is important that the gap tracker of the ram loader is in a good
    // state.
    //
    // a. If this is on exit, we can simply interrupt reading and use the ram
    // loader's current gap tracker, if any, because a precondition of this
    // function running is that we are saving the same snapshot as we have
    // loaded.
    //
    // b. If this is not on exit, then we first need to make the loader finish
    // whatever it was doing, because the emulator will need valid ram after
    // the save. Then, we also need to invalidate the loader's gap tracker
    // because it's unknown whether the particular version of this loader's
    // gaps corresponds properly to the gaps in the |kRamFileName| file on disk
    // (e.g., we might have saved more than once after a load).

    if (mRamLoader && !mRamLoader->hasError()) {
        if (isOnExit) {
            mRamLoader->interrupt();
        } else {
            mRamLoader->join();
            mRamLoader->invalidateGaps();
        }

        // If we transitioned from file backed to non-file-backed, we will
        // need to rewrite the index and cannot use a previous index.
        // Otherwise, there will be a lot of confusing cases to handle.
        if (mRamLoader->didSwitchFileBacking()) {
            mRamLoader.clear();
            return;
        }

        if (!mRamLoader->hasGaps()) {
            const auto ram = ::android_fopen(
                    PathUtils::join(mSnapshot.dataDir(), kRamFileName).c_str(), "rb");

            if (!ram) return;

            mRamLoader.emplace(
                    StdioStream(ram, StdioStream::kOwner),
                    RamLoader::Flags::LoadIndexOnly,
                    mRamLoader->getRamBlockStructure());
        }

    }
}

}  // namespace snapshot
}  // namespace android
