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

#include "android/snapshot/PathUtils.h"

#include "android/avd/info.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/globals.h"
#include "android/utils/path.h"

static android::base::LazyInstance<std::string> s_snapshotName =
        LAZY_INSTANCE_INIT;

namespace android {
namespace snapshot {

void setSnapshotName(const char* snapshotName) {
    s_snapshotName.get() = snapshotName;
}

std::string getSnapshotDir(bool create) {
    return getSnapshotDir(s_snapshotName->c_str(), create);
}

std::string getSnapshotDir(const char* snapshotName, bool create) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path = base::PathUtils::join(dir, "snapshots", snapshotName);
    if (create) {
        if (path_mkdir_if_needed(path.c_str(), 0777) != 0) {
            return {};
        }
    }
    return path;
}

}  // namespace snapshot
}  // namespace android
