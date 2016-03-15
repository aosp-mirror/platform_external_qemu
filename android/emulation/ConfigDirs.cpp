// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/ConfigDirs.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"

#include <assert.h>

namespace android {

using ::android::base::PathUtils;
using ::android::base::ScopedCPtr;
using ::android::base::System;

// static
std::string ConfigDirs::getUserDirectory() {
    // Name of the Android configuration directory under $HOME.
    static const char kAndroidSubDir[] = ".android";
    System* system = System::get();
    std::string home = system->envGet("ANDROID_EMULATOR_HOME");
    if (home.size()) {
        return home;
    }
    home = system->envGet("ANDROID_SDK_HOME");
    if (home.size()) {
        return PathUtils::join(home, kAndroidSubDir);
    }
    home = system->getHomeDirectory();
    if (home.empty()) {
        home = system->getTempDir();
        if (home.empty()) {
            home = "/tmp";
        }
    }
    return PathUtils::join(home, kAndroidSubDir);
}

// static
std::string ConfigDirs::getAvdRootDirectory() {
    static const char kAvdSubDir[] = "avd";
    System* system = System::get();
    std::string home = system->envGet("ANDROID_AVD_HOME");
    if (home.size()) {
        return home;
    }
    std::string result = PathUtils::join(getUserDirectory(), kAvdSubDir);
    return result;
}

// static
std::string ConfigDirs::getSdkRootDirectoryByEnv() {
    auto system = System::get();
    std::string sdkRoot = system->envGet("ANDROID_SDK_ROOT");

    if (sdkRoot.size()) {
        // Unquote a possibly "quoted" path.
        if (sdkRoot[0] == '"') {
            assert(sdkRoot[sdkRoot.size() - 1] == '"');
            // std::string does not support assigning a part of the string to
            // itself.
            auto copySize = sdkRoot.size() - 2;
            ScopedCPtr<char> buf(android::base::strDup(sdkRoot));
            sdkRoot.assign(buf.get() + 1, copySize);
        }
        if (system->pathIsDir(sdkRoot) && system->pathCanRead(sdkRoot)) {
            return sdkRoot;
        }
    }
    return std::string();
}

std::string ConfigDirs::getSdkRootDirectoryByPath() {
    auto system = System::get();

    auto parts = PathUtils::decompose(system->getLauncherDirectory());
    parts.push_back("..");
    PathUtils::simplifyComponents(&parts);

    std::string sdkRoot = PathUtils::recompose(parts);
    if (system->pathIsDir(sdkRoot) && system->pathCanRead(sdkRoot)) {
        return sdkRoot;
    }
    return std::string();
}

// static
std::string ConfigDirs::getSdkRootDirectory() {
    std::string sdkRoot = getSdkRootDirectoryByEnv();
    if (!sdkRoot.empty()) {
        return sdkRoot;
    }

    // Otherwise, infer from the path of the emulator's binary.
    return getSdkRootDirectoryByPath();
}

}  // namespace android
