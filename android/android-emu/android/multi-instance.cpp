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
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/utils/debug.h"

static bool sInitialized = false;
static android::base::FileShare sShareMode = android::base::FileShare::Write;
static FILE* sSharedFile = nullptr;

bool android::instanceShare::initInstanceShareMode(
        android::base::FileShare shareMode) {
    if (sInitialized) {
        return sShareMode == shareMode;
    }
    AvdInfo* avd = android_avdInfo;
    const char* multiInstanceLock = avdInfo_getMultiInstanceLockFilePath(avd);
    if (!android::base::System::get()->pathIsFile(multiInstanceLock)) {
        android::base::createFileForShare(multiInstanceLock);
    }
    const char* mode = sShareMode == android::base::FileShare::Read ? "rb" : "wb";
    sSharedFile = android::base::fsopen(multiInstanceLock, mode, shareMode);
    if (!sSharedFile) {
        derror("Another emulator instance is running. Please close it or "
                "run all emulators with -read-only flag.\n");
        return false;
    }
    sInitialized = true;
    sShareMode = shareMode;
    return true;
}

bool android::instanceShare::updateInstanceShareMode(base::FileShare shareMode) {
    assert(sInitialized);
    return android::base::updateFileShare(sSharedFile, shareMode);
}
