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

#include "android/base/StringFormat.h"
#include "android/base/system/System.h"

namespace android {

using ::android::base::String;
using ::android::base::StringFormat;
using ::android::base::System;

// static
String ConfigDirs::getUserDirectory() {
    // Name of the Android configuration directory under $HOME.
    static const char kAndroidSubDir[] = ".android";
    System* system = System::get();
    String home = system->envGet("ANDROID_EMULATOR_HOME");
    if (home.size()) {
        return home;
    }
    home = system->envGet("ANDROID_SDK_HOME");
    if (home.size()) {
        return StringFormat("%s%c%s", home.c_str(), System::kDirSeparator,
                            kAndroidSubDir);
    }
    home = system->getHomeDirectory();
    if (home.empty()) {
        home = system->getTempDir();
        if (home.empty()) {
            home = "/tmp";
        }
    }
    return StringFormat("%s%c%s", home.c_str(), System::kDirSeparator,
                        kAndroidSubDir);
}

// static
String ConfigDirs::getAvdRootDirectory() {
    static const char kAvdSubDir[] = "avd";
    System* system = System::get();
    String home = system->envGet("ANDROID_AVD_HOME");
    if (home.size()) {
        return home;
    }
    String result = getUserDirectory();
    result += System::kDirSeparator;
    result += kAvdSubDir;
    return result;
}

}  // namespace android
