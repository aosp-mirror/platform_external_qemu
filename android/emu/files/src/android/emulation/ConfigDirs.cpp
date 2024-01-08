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

#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string_view>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

namespace android {

using ::android::base::AutoLock;
using ::android::base::LazyInstance;
using ::android::base::Lock;
using ::android::base::PathUtils;
using ::android::base::pj;
using ::android::base::System;

// Name of the Android configuration directory under $HOME.
static const char kAndroidSubDir[] = ".android";
// Subdirectory for AVD data files.
static const char kAvdSubDir[] = "avd";

// static
auto ConfigDirs::getUserDirectory() -> std::string {
    System* system = System::get();
    std::string home = system->envGet("ANDROID_EMULATOR_HOME");
    if (!home.empty()) {
        return home;
    }

    // New key: ANDROID_PREFS_ROOT
    home = system->envGet("ANDROID_PREFS_ROOT");
    if (!home.empty()) {
        // In v1.9 emulator was changed to use $ANDROID_SDK_HOME/.android
        // directory, but Android Studio has always been using $ANDROID_SDK_HOME
        // directly. Put a workaround here to make sure it works both ways,
        // preferring the one from AS.
        auto homeNewWay = PathUtils::join(home, kAndroidSubDir);
        return system->pathIsDir(homeNewWay) ? homeNewWay : home;
    }  // Old key that is deprecated (ANDROID_SDK_HOME)
    home = system->envGet("ANDROID_SDK_HOME");
    if (!home.empty()) {
        auto homeOldWay = PathUtils::join(home, kAndroidSubDir);
        return system->pathIsDir(homeOldWay) ? homeOldWay : home;
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
auto ConfigDirs::getAvdRootDirectory() -> std::string {
    System* system = System::get();

    // The search order here should match that in AndroidLocation.java
    // in Android Studio. Otherwise, Studio and the Emulator may find
    // different AVDs. Or one may find an AVD when the other doesn't.
    std::string avdRoot = system->envGet("ANDROID_AVD_HOME");
    if (!avdRoot.empty() && system->pathIsDir(avdRoot)) {
        return avdRoot;
    }

    // No luck with ANDROID_AVD_HOME, try ANDROID_PREFS_ROOT/ANDROID_SDK_HOME
    avdRoot = getAvdRootDirectoryWithPrefsRoot(
            system->envGet("ANDROID_PREFS_ROOT"));
    if (!avdRoot.empty()) {
        return avdRoot;
    }
    avdRoot = getAvdRootDirectoryWithPrefsRoot(
            system->envGet("ANDROID_SDK_HOME"));
    if (!avdRoot.empty()) {
        return avdRoot;
    }  // ANDROID_PREFS_ROOT/ANDROID_SDK_HOME is defined but bad. In this case,
    // Android Studio tries $TEST_TMPDIR, $USER_HOME, and
    // $HOME. We'll do the same.
    avdRoot = system->envGet("TEST_TMPDIR");
    if (!avdRoot.empty()) {
        avdRoot = PathUtils::join(avdRoot, kAndroidSubDir);
        if (isValidAvdRoot(avdRoot)) {
            return PathUtils::join(avdRoot, kAvdSubDir);
        }
    }
    avdRoot = system->envGet("USER_HOME");
    if (!avdRoot.empty()) {
        avdRoot = PathUtils::join(avdRoot, kAndroidSubDir);
        if (isValidAvdRoot(avdRoot)) {
            return PathUtils::join(avdRoot, kAvdSubDir);
        }
    }
    avdRoot = system->envGet("HOME");
    if (!avdRoot.empty()) {
        avdRoot = PathUtils::join(avdRoot, kAndroidSubDir);
        if (isValidAvdRoot(avdRoot)) {
            return PathUtils::join(avdRoot, kAvdSubDir);
        }
    }

    // No luck with ANDROID_AVD_HOME, ANDROID_SDK_HOME,
    // TEST_TMPDIR, USER_HOME, or HOME. Try even more.
    avdRoot = PathUtils::join(getUserDirectory(), kAvdSubDir);
    return avdRoot;
}

// static
auto ConfigDirs::getSdkRootDirectoryByEnv(bool verbose) -> std::string {
    auto* system = System::get();

    if (verbose) {
        dinfo("checking ANDROID_HOME for valid sdk root.");
    }
    std::string sdkRoot = system->envGet("ANDROID_HOME");
    if ((static_cast<unsigned int>(!sdkRoot.empty()) != 0U) &&
        isValidSdkRoot(sdkRoot, verbose)) {
        return sdkRoot;
    }

    if (verbose) {
        dinfo("checking ANDROID_SDK_ROOT for valid sdk root.");
    }
    // ANDROID_HOME is not good. Try ANDROID_SDK_ROOT.
    sdkRoot = system->envGet("ANDROID_SDK_ROOT");
    if (static_cast<unsigned int>(!sdkRoot.empty()) != 0U) {
        // Unquote a possibly "quoted" path.
        if (sdkRoot[0] == '"') {
            assert(sdkRoot.back() == '"');
            sdkRoot.erase(0, 1);
            sdkRoot.pop_back();
        }
        if (isValidSdkRoot(sdkRoot, verbose)) {
            return sdkRoot;
        }
    } else if (verbose) {
        dwarning("ANDROID_SDK_ROOT is missing.");
    }

    return {};
}

auto ConfigDirs::getSdkRootDirectoryByPath(bool verbose) -> std::string {
    auto* system = System::get();

    auto parts = PathUtils::decompose(system->getLauncherDirectory());

    std::string sdkRoot = PathUtils::recompose(parts);
    for (int i = 0; i < 3; ++i) {
        parts.emplace_back("..");
        PathUtils::simplifyComponents(&parts);

        sdkRoot = PathUtils::recompose(parts);
        if (verbose) {
            dinfo("guessed sdk root is %s", sdkRoot);
        }
        if (isValidSdkRoot(sdkRoot, verbose)) {
            return sdkRoot;
        }
        if (verbose) {
            dinfo("guessed sdk root %s does not seem to be valid",
                  sdkRoot.c_str());
        }
    }
    if (verbose) {
        dwarning("invalid sdk root %s", sdkRoot);
    }
    return {};
}

// static
auto ConfigDirs::getSdkRootDirectory(bool verbose) -> std::string {
    std::string sdkRoot = getSdkRootDirectoryByEnv(verbose);
    if (!sdkRoot.empty()) {
        return sdkRoot;
    }

    if (verbose) {
        printf("emulator: WARN: Cannot find valid sdk root from environment "
               "variable ANDROID_HOME nor ANDROID_SDK_ROOT,"
               "Try to infer from emulator's path\n");
    }
    // Otherwise, infer from the path of the emulator's binary.
    return getSdkRootDirectoryByPath(verbose);
}

// static
auto ConfigDirs::isValidSdkRoot(const std::string& rootPath, bool verbose)
        -> bool {
    if (rootPath.empty()) {
        if (verbose) {
            printf("emulator: WARN: empty sdk root\n");
        }
        return false;
    }
    System* system = System::get();
    if (!system->pathIsDir(rootPath) || !system->pathCanRead(rootPath)) {
        if (verbose) {
            if (!system->pathIsDir(rootPath)) {
                printf("emulator: WARN: %s is not a directory, and canot be "
                       "sdk root\n",
                       rootPath.c_str());
            } else if (!system->pathCanRead(rootPath)) {
                printf("emulator: WARN: %s is not readable, and canot be sdk "
                       "root\n",
                       rootPath.c_str());
            }
        }
        return false;
    }
    std::string platformsPath = PathUtils::join(rootPath, "platforms");
    if (!system->pathIsDir(platformsPath)) {
        if (verbose) {
            dwarning(
                    "platforms subdirectory is missing under "
                    "%s, please install it",
                    rootPath.c_str());
        }
        return false;
    }
    std::string platformToolsPath = PathUtils::join(rootPath, "platform-tools");
    if (!system->pathIsDir(platformToolsPath)) {
        if (verbose) {
            dwarning(
                    "platform-tools subdirectory is missing "
                    "under %s, please install it",
                    rootPath.c_str());
        }
        return false;
    }

    return true;
}

// static
auto ConfigDirs::isValidAvdRoot(const std::string_view& avdPath) -> bool {
    if (avdPath.empty()) {
        return false;
    }
    System* system = System::get();
    if (!system->pathIsDir(avdPath) || !system->pathCanRead(avdPath)) {
        return false;
    }
    std::string avdAvdPath = PathUtils::join(avdPath.data(), "avd");
    return system->pathIsDir(avdAvdPath);
}

auto ConfigDirs::getAvdRootDirectoryWithPrefsRoot(const std::string& path)
        -> std::string {
    if (path.empty()) {
        return {};
    }

    // ANDROID_PREFS_ROOT is defined
    if (isValidAvdRoot(path)) {
        // ANDROID_PREFS_ROOT is good
        return PathUtils::join(path, kAvdSubDir);
    }

    std::string avdRoot = PathUtils::join(path, kAndroidSubDir);
    if (isValidAvdRoot(avdRoot)) {
        // ANDROID_PREFS_ROOT/.android is good
        return PathUtils::join(avdRoot, kAvdSubDir);
    }

    return {};
}

using discovery_dir = struct discovery_dir {
    const char* root_env;
    const char* subdir;
};

#if defined(_WIN32)
discovery_dir discovery{"LOCALAPPDATA", "Temp"};
#elif defined(__linux__)
discovery_dir discovery{"XDG_RUNTIME_DIR", ""};
#elif defined(__APPLE__)
discovery_dir discovery{"HOME", "Library/Caches/TemporaryItems"};
#else
#error This platform is not supported.
#endif

static auto getAlternativeRoot() -> std::string {
#ifdef __linux__
    auto uid = getuid();
    auto discovery = pj("/run/user/", std::to_string(uid));
    if (System::get()->pathExists(discovery)) {
        return discovery;
    }
#endif

    // Reverting to the standard emulator user directories
    return ConfigDirs::getUserDirectory();
}

auto ConfigDirs::getDiscoveryDirectory() -> std::string {
    auto root = System::get()->getEnvironmentVariable(discovery.root_env);
    if (root.empty()) {
        // Reverting to the alternative root if these environment variables do
        // not exist.
        dwarning(
                "Using fallback path for the emulator registration "
                "directory.");
        root = getAlternativeRoot();
    } else {
        root = pj(root, discovery.subdir);
    }

    auto desired_directory = pj({root, "avd", "running"});
    auto path = PathUtils::decompose(desired_directory);
    PathUtils::simplifyComponents(&path);
    auto recomposed = PathUtils::recompose(path);
    if (!System::get()->pathExists(recomposed)) {
        if (path_mkdir_if_needed(recomposed.c_str(), 0700) == -1) {
            dinfo("Unable to create %s due to %d: %s", recomposed,
                  errno, strerror(errno));
        }
    }
    return recomposed;
}



}  // namespace android
