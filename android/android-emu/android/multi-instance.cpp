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

#include "android/multi-instance.h"

#include "android/base/files/FileShareOpen.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/utils/debug.h"

namespace {
struct MultiInstanceState {
    android::base::FileShare shareMode = android::base::FileShare::Write;
    FILE* sharedFile = nullptr;
    android::multiinstance::UpdateDriveShareModeFunc updateDriveShareModeFunc =
            nullptr;
};
static android::base::LazyInstance<MultiInstanceState> sMultiInstanceState =
        LAZY_INSTANCE_INIT;
}  // namespace

bool android::multiinstance::initInstanceShareMode(
        android::base::FileShare shareMode) {
    if (sMultiInstanceState.hasInstance()) {
        return sMultiInstanceState->shareMode == shareMode;
    }
    AvdInfo* avd = android_avdInfo;
    const char* multiInstanceLock = avdInfo_getMultiInstanceLockFilePath(avd);
    if (!android::base::System::get()->pathIsFile(multiInstanceLock)) {
        android::base::createFileForShare(multiInstanceLock);
    }
    const char* mode =
            shareMode == android::base::FileShare::Read
                    ? "rb"
                    : "wb";
    sMultiInstanceState->sharedFile =
            android::base::fsopen(multiInstanceLock, mode, shareMode);
    if (!sMultiInstanceState->sharedFile) {
        derror("Another emulator instance is running. Please close it or "
               "run all emulators with -read-only flag.\n");
        return false;
    }
    sMultiInstanceState->shareMode = shareMode;
    return true;
}

bool android::multiinstance::updateInstanceShareMode(
        const char* snapshotName,
        base::FileShare shareMode) {
    assert(sMultiInstanceState.hasInstance());
    if (sMultiInstanceState->shareMode == shareMode) {
        return true;
    }
    if (android::base::updateFileShare(sMultiInstanceState->sharedFile,
                                       shareMode)) {
        if (sMultiInstanceState->updateDriveShareModeFunc) {
            if (!sMultiInstanceState->updateDriveShareModeFunc(snapshotName,
                                                               shareMode)) {
                return false;
            }
        }
        sMultiInstanceState->shareMode = shareMode;
        return true;
    } else {
        return false;
    }
}

void android::multiinstance::setUpdateDriveShareModeFunc(
        UpdateDriveShareModeFunc updateDriveShareModeFunc) {
    sMultiInstanceState->updateDriveShareModeFunc = updateDriveShareModeFunc;
}

android::base::FileShare android::multiinstance::getInstanceShareMode() {
    assert(sMultiInstanceState.hasInstance());
    return sMultiInstanceState->shareMode;
}
