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
#include "android/emulation/ComponentVersion.h"

#include "aemu/base/files/PathUtils.h"

#include <cstdio>
#include <fstream>
#include <string_view>

namespace android {

using android::base::PathUtils;
using android::base::Version;

static Version parseVersionFromSourceProperties(
        const std::string& propertiesPath) {
    Version version = Version::invalid();
    std::ifstream propertiesFile(PathUtils::asUnicodePath(propertiesPath.data()).c_str());
    if (propertiesFile.is_open()) {
        // Find the line containing "Pkg.Revision".
        std::string line;
        char verStr[100];
        while (std::getline(propertiesFile, line)) {
            if (sscanf(line.c_str(), " Pkg.Revision = %s", verStr) == 1) {
                version = Version(verStr);
                break;
            }
        }
    }
    return version;
}

static std::string getComponentPath(std::string_view sdkRootDirectory,
                                    SdkComponentType type) {
    switch (type) {
        case SdkComponentType::PlatformTools:
            return PathUtils::join(sdkRootDirectory.data(), "platform-tools");
        case SdkComponentType::Tools:
            return PathUtils::join(sdkRootDirectory.data(), "tools");
        default:
            return "";
    }
}

android::base::Version getCurrentSdkVersion(std::string_view sdkRootDirectory,
                                            SdkComponentType type) {
    Version version = Version::invalid();
    if (!sdkRootDirectory.empty()) {
        std::string componentPath = getComponentPath(sdkRootDirectory, type);
        if (!componentPath.empty()) {
            version = parseVersionFromSourceProperties(
                    PathUtils::join(componentPath, "source.properties"));
        }
    }
    return version;
}

}  // namespace android
