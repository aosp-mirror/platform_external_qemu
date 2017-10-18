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
#include "android/base/system/System.h"
#include "android/globals.h"

using android::base::System;

namespace android {
namespace snapshot {

std::string getSnapshotsDir() {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    return base::PathUtils::join(dir, "snapshots");
}

std::string getSnapshotDepsFileName() {
    return base::PathUtils::join(getSnapshotsDir(), "snapshot_deps.pb");
}

std::string getSnapshotDir(const char* snapshotName) {
    return base::PathUtils::join(getSnapshotsDir(), snapshotName);
}

std::vector<std::string> getSnapshotDirEntries() {
    std::vector<std::string> dirs =
        System::get()->scanDirEntries(getSnapshotsDir());

    fprintf(stderr, "%s: %zu\n", __func__, dirs.size());
    for (const auto& elt : dirs) {
        fprintf(stderr, "%s: %s\n", __func__, elt.c_str());
    }

    return dirs;
}

}  // namespace snapshot
}  // namespace android
