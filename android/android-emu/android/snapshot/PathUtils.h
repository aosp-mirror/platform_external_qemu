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

#pragma once

#include "android/base/system/System.h"

#include <string>
#include <vector>

namespace android {
namespace snapshot {

std::string getSnapshotBaseDir();
std::string getSnapshotDir(const char* snapshotName);
std::string getSnapshotDepsFileName();
std::vector<std::string> getSnapshotDirEntries();
std::vector<std::string> getQcow2Files(std::string avdDir);
std::string getAvdDir();
std::string getQuickbootChoiceIniPath();

base::System::FileSize folderSize(const std::string& snapshotName);

}  // namespace snapshot
}  // namespace android
