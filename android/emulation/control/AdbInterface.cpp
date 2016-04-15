// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/AdbInterface.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"

#include <cstdio>
#include <fstream>
#include <string>

using namespace android::base;

namespace android {
namespace emulation {

// Helper function, checks if the version of adb in the given SDK is
// fresh enough.
static bool checkAdbVersion(const std::string& sdk_root_directory) {
    static const int kMinAdbVersionMajor = 23;
    static const int kMinAdbVersionMinor = 1;

    if (sdk_root_directory.empty()) {
        return false;
    }

    // The file at $(ANDROID_SDK_ROOT)/platform-tools/source.properties tells
    // what version the ADB executable is. Find that file.
    std::string properties_path =
        PathUtils::join(sdk_root_directory,
                        "platform-tools",
                        "source.properties");

    std::ifstream properties_file(properties_path.c_str());
    if (properties_file) {
        // Find the line containing "Pkg.Revision".
        std::string line;
        while (std::getline(properties_file, line)) {
            int version_major, version_minor = 0;
            if (sscanf(line.c_str(),
                       " Pkg.Revision = %d.%d",
                       &version_major,
                       &version_minor) >= 1) {
                return  version_major > kMinAdbVersionMajor ||
                       (version_major == kMinAdbVersionMajor &&
                        version_minor >= kMinAdbVersionMinor);
            }
        }
    }

    // If the file is missing, assume the tools directory is broken in some
    // way, and updating should fix the problem.
    return false;
}

AdbInterface::AdbInterface() : mAdbVersionCurrent(false) {
    // First try finding ADB by the environment variable.
    auto sdk_root_by_env = android::ConfigDirs::getSdkRootDirectoryByEnv();
    if (!sdk_root_by_env.empty()) {
        // If ANDROID_SDK_ROOT is defined, the user most likely wanted to use
        // that ADB. Store it for later - if the second potential ADB path
        // also fails, we'll warn the user about this one.
        mAdbPath =
            PathUtils::join(sdk_root_by_env, "platform-tools", "adb");
        if (checkAdbVersion(sdk_root_by_env)) {
            mAdbVersionCurrent = true;
            return;
        }
    }

    // If the first path was non-existent or a bad version, try to infer the
    // path based on the emulator executable location.
    auto sdk_root_by_path = android::ConfigDirs::getSdkRootDirectoryByPath();
    if (sdk_root_by_path != sdk_root_by_env &&
        !sdk_root_by_path.empty()) {
        if (checkAdbVersion(sdk_root_by_path)) {
            mAdbPath =
                PathUtils::join(sdk_root_by_path, "platform-tools", "adb");
            mAdbVersionCurrent = true;
            return;

            // Only save this path if the ANDROID_SDK_ROOT path was not set.
        } else if (mAdbPath.empty()) {
            mAdbPath =
                PathUtils::join(sdk_root_by_path, "platform-tools", "adb");
        }
    }
}

}
}
