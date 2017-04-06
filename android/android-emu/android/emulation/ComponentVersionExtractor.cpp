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
#include "android/emulation/ComponentVersionExtractor.h"

#include "android/base/files/PathUtils.h"

#include <cstdio>
#include <fstream>

namespace android {

using android::base::Version;
using android::base::PathUtils;
using android::base::StringView;

static Version parseVersionFromSourceProperties(std::string& propertiesPath) {
    Version version = Version::invalid();
    std::ifstream propertiesFile(propertiesPath.c_str());
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
    propertiesFile.close();
    return version;
}

Version ComponentVersionExtractor::getSdkVersion(SdkComponentType type) {
    if (mCachedVersions.find(type) != mCachedVersions.end()) {
        return mCachedVersions[type];
    }

    Version version = Version::invalid();

    if (mSdkRootDirectory.empty()) {
        return version;
    }
    std::string propertiesPath;

    switch (type) {
        case SDK_PLATFORM_TOOLS:
            propertiesPath = PathUtils::join(
                    mSdkRootDirectory, "platform-tools", "source.properties");
            version = parseVersionFromSourceProperties(propertiesPath);
            mCachedVersions[type] = version;
            break;
        case SDK_TOOLS:
            propertiesPath = PathUtils::join(mSdkRootDirectory, "tools",
                                             "source.properties");
            version = parseVersionFromSourceProperties(propertiesPath);
            mCachedVersions[type] = version;
            break;
        default:;
    }
    return version;
}

}  // namespace android