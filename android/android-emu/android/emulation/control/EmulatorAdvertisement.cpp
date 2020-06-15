// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/emulation/control/EmulatorAdvertisement.h"

#include <stdio.h>                         // for sscanf
#include <sys/stat.h>                      // for chmod
#include <fstream>                         // for operator<<, basic_ofstream
#include <utility>                         // for pair
#include <vector>                          // for vector

#include "android/base/Log.h"              // for LOG, LogMessage, LogStream
#include "android/base/StringFormat.h"     // for StringFormat
#include "android/base/files/PathUtils.h"  // for pj
#include "android/base/system/System.h"    // for System, System::WaitExitRe...
#include "android/emulation/ConfigDirs.h"  // for ConfigDirs
#include "android/globals.h"

namespace android {
namespace emulation {
namespace control {

using android::base::System;
using android::base::PathUtils;
static const char* location_format = "pid_%d.ini";


EmulatorAdvertisement::EmulatorAdvertisement(EmulatorProperties&& config)
    : mStudioConfig(config) {
    mSharedDirectory = android::ConfigDirs::getDiscoveryDirectory();
}

EmulatorAdvertisement::EmulatorAdvertisement(EmulatorProperties&& config,
                                             std::string sharedDirectory)
    : mStudioConfig(config), mSharedDirectory(sharedDirectory) {}

EmulatorAdvertisement::~EmulatorAdvertisement() {
    garbageCollect();
}

int EmulatorAdvertisement::garbageCollect() const {
    int collected = 0;
    for (auto entry : System::get()->scanDirEntries(mSharedDirectory)) {
        int pid = 0;
        if (sscanf(entry.c_str(), location_format, &pid) == 1) {
            auto status = System::get()->waitForProcessExit(pid, 0);
            if (status != System::WaitExitResult::Timeout) {
                collected++;
                // pid is not running or unavailable.
                System::get()->deleteFile(
                        android::base::pj(mSharedDirectory, entry));
            }
        }
    }

    return collected;
}

void EmulatorAdvertisement::remove() const {
    System::get()->deleteFile(location());
}

std::string EmulatorAdvertisement::location() const {
    auto pid = System::get()->getCurrentProcessId();
    std::string pidfile = android::base::StringFormat(location_format, pid);
    std::string result = android::base::pj(mSharedDirectory, pidfile);
    android::ConfigDirs::setCurrentDiscoveryPath(result);
    return result;
}

bool EmulatorAdvertisement::write()  const {
    auto pidFile = location();
    if (System::get()->pathExists(pidFile)) {
        LOG(WARNING) << "Overwriting existing discovery file: " << pidFile;
    }
    auto shareFile = std::ofstream(PathUtils::asUnicodePath(pidFile).c_str());
    for (const auto& elem : mStudioConfig) {
        shareFile << elem.first << "=" << elem.second << "\n";
    }
    shareFile.close();

#ifndef _WIN32
    // To ensure that your files are not removed, they should have their access
    // time timestamp modified at least once every 6 hours of monotonic time or
    // the 'sticky' bit should be set on the file.
    chmod(pidFile.c_str(), S_IRUSR | S_ISVTX | S_IWUSR);
#endif
    return !shareFile.bad();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
