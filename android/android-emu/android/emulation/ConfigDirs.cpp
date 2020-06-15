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

#include <assert.h>  // for assert
#include <errno.h>   // for errno
#include <string.h>  // for strerror

#include <vector>  // for vector

#include "android/base/Log.h"              // for LogStream, LOG, LogMessage
#include "android/base/Optional.h"         // for Optional
#include "android/base/files/PathUtils.h"  // for PathUtils, pj
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"    // for System
#include "android/utils/path.h"            // for path_mkdir_if_needed

namespace android {

using ::android::base::AutoLock;;
using ::android::base::LazyInstance;
using ::android::base::Lock;
using ::android::base::PathUtils;
using ::android::base::pj;
using ::android::base::System;

// Name of the Android configuration directory under $HOME.
static const char kAndroidSubDir[] = ".android";
// Subdirectory for AVD data files.
static const char kAvdSubDir[] = "avd";

// Current discovery path
class DiscoveryPathState {
public:
    Lock lock;
    std::string path = "";
};

static LazyInstance<DiscoveryPathState> sDiscoveryPathState =
    LAZY_INSTANCE_INIT;

// static
std::string ConfigDirs::getUserDirectory() {
    System* system = System::get();
    std::string home = system->envGet("ANDROID_EMULATOR_HOME");
    if (!home.empty()) {
        return home;
    }

    home = system->envGet("ANDROID_SDK_HOME");
    if (!home.empty()) {
        // In v1.9 emulator was changed to use $ANDROID_SDK_HOME/.android
        // directory, but Android Studio has always been using $ANDROID_SDK_HOME
        // directly. Put a workaround here to make sure it works both ways,
        // preferring the one from AS.
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
std::string ConfigDirs::getAvdRootDirectory() {
    System* system = System::get();

    // The search order here should match that in AndroidLocation.java
    // in Android Studio. Otherwise, Studio and the Emulator may find
    // different AVDs. Or one may find an AVD when the other doesn't.

    std::string avdRoot = system->envGet("ANDROID_AVD_HOME");
    if (!avdRoot.empty() && system->pathIsDir(avdRoot)) {
        return avdRoot;
    }

    // No luck with ANDROID_AVD_HOME, try ANDROID_SDK_HOME
    avdRoot = system->envGet("ANDROID_SDK_HOME");
    if (!avdRoot.empty()) {
        // ANDROID_SDK_HOME is defined
        if (isValidAvdRoot(avdRoot)) {
            // ANDROID_SDK_HOME is good
            return PathUtils::join(avdRoot, kAvdSubDir);
        }
        avdRoot = PathUtils::join(avdRoot, kAndroidSubDir);
        if (isValidAvdRoot(avdRoot)) {
            // ANDROID_SDK_HOME/.android is good
            return PathUtils::join(avdRoot, kAvdSubDir);
        }
        // ANDROID_SDK_HOME is defined but bad. In this case,
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
    }

    // No luck with ANDROID_AVD_HOME, ANDROID_SDK_HOME,
    // TEST_TMPDIR, USER_HOME, or HOME. Try even more.
    avdRoot = PathUtils::join(getUserDirectory(), kAvdSubDir);
    return avdRoot;
}

// static
std::string ConfigDirs::getSdkRootDirectoryByEnv() {
    auto system = System::get();

    std::string sdkRoot = system->envGet("ANDROID_HOME");
    if (isValidSdkRoot(sdkRoot)) {
        return sdkRoot;
    }

    // ANDROID_HOME is not good. Try ANDROID_SDK_ROOT.
    sdkRoot = system->envGet("ANDROID_SDK_ROOT");
    if (sdkRoot.size()) {
        // Unquote a possibly "quoted" path.
        if (sdkRoot[0] == '"') {
            assert(sdkRoot.back() == '"');
            sdkRoot.erase(0, 1);
            sdkRoot.pop_back();
        }
        if (isValidSdkRoot(sdkRoot)) {
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
    if (isValidSdkRoot(sdkRoot)) {
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

// static
bool ConfigDirs::isValidSdkRoot(android::base::StringView rootPath) {
    if (rootPath.empty()) {
        return false;
    }
    System* system = System::get();
    if (!system->pathIsDir(rootPath) || !system->pathCanRead(rootPath)) {
        return false;
    }
    std::string platformsPath = PathUtils::join(rootPath, "platforms");
    if (!system->pathIsDir(platformsPath)) {
        return false;
    }
    std::string platformToolsPath = PathUtils::join(rootPath, "platform-tools");
    if (!system->pathIsDir(platformToolsPath)) {
        return false;
    }

    return true;
}

// static
bool ConfigDirs::isValidAvdRoot(android::base::StringView avdPath) {
    if (avdPath.empty()) {
        return false;
    }
    System* system = System::get();
    if (!system->pathIsDir(avdPath) || !system->pathCanRead(avdPath)) {
        return false;
    }
    std::string avdAvdPath = PathUtils::join(avdPath, "avd");
    if (!system->pathIsDir(avdAvdPath)) {
        return false;
    }

    return true;
}

typedef struct discovery_dir {
    const char* root_env;
    const char* subdir;
} discovery_dir;

#if defined(_WIN32)
discovery_dir discovery{"LOCALAPPDATA", "Temp"};
#elif defined(__linux__)
discovery_dir discovery{"XDG_RUNTIME_DIR", ""};
#elif defined(__APPLE__)
discovery_dir discovery{"HOME", "Library/Caches/TemporaryItems"};
#else
#error This platform is not supported.
#endif

static std::string getAlternativeRoot() {
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

std::string ConfigDirs::getDiscoveryDirectory() {
    auto root = System::get()->getEnvironmentVariable(discovery.root_env);
    if (root.empty()) {
        // Reverting to the alternative root if these environment variables do
        // not exist.
        fprintf(stderr,
                "WARNING. Using fallback path for the emulator registration "
                "directory.\n");
        root = getAlternativeRoot();
    } else {
        root = pj(root, discovery.subdir);
    }

    auto desired_directory = pj(root, "avd", "running");
    auto path = PathUtils::decompose(desired_directory);
    PathUtils::simplifyComponents(&path);
    auto recomposed = PathUtils::recompose(path);
    if (!System::get()->pathExists(recomposed)) {
        if (path_mkdir_if_needed(recomposed.c_str(), 0700) == -1) {
            LOG(ERROR) << "Unable to create " << recomposed << " due to "
                       << errno << ": " << strerror(errno);
        }
    }
    return recomposed;
}

void ConfigDirs::setCurrentDiscoveryPath(android::base::StringView path) {
    AutoLock lock(sDiscoveryPathState->lock);
    sDiscoveryPathState->path = path.str();
}

std::string ConfigDirs::getCurrentDiscoveryPath() {
    AutoLock lock(sDiscoveryPathState->lock);
    return sDiscoveryPathState->path;
}

}  // namespace android
