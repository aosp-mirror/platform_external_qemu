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

std::string getSnapshotBaseDir() {
    auto avdDir = avdInfo_getContentPath(android_avdInfo);
    auto path = base::PathUtils::join(avdDir, "snapshots");
    return path;
}

std::string getSnapshotDir(const char* snapshotName) {
    auto baseDir = getSnapshotBaseDir();
    auto path = base::PathUtils::join(baseDir, snapshotName);
    return path;
}

std::string getSnapshotDepsFileName() {
    return base::PathUtils::join(getSnapshotBaseDir(), "snapshot_deps.pb");
}

std::vector<std::string> getSnapshotDirEntries() {
    return System::get()->scanDirEntries(getSnapshotBaseDir());
}

base::System::FileSize folderSize(const std::string& snapshotName) {
    return System::get()->recursiveSize(
            base::PathUtils::join(getSnapshotBaseDir(), snapshotName));
}

std::string getQuickbootChoiceIniPath() {
    auto avdDir = avdInfo_getContentPath(android_avdInfo);
    auto path = base::PathUtils::join(avdDir, "quickbootChoice.ini");
    return path;
}

}  // namespace snapshot
}  // namespace android
