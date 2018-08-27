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

#pragma once

#include <string>

#include "android/base/files/FileShareOpen.h"

namespace android {
namespace multiinstance {

typedef bool (*UpdateDriveShareModeFunc)(const char* snapshotName,
                                         base::FileShare shareMode);

extern bool initInstanceShareMode(base::FileShare shareMode);
extern bool updateInstanceShareMode(const char* snapshotName,
                                    base::FileShare shareMode);
extern void setUpdateDriveShareModeFunc(
        UpdateDriveShareModeFunc updateDriveShareModeFunc);
extern base::FileShare getInstanceShareMode();

}  // namespace multiinstance
}  // namespace android
