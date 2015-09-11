/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/utils/bufprint.h"

#include "android/base/String.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

using android::base::String;
using android::base::System;

// Implementation of bufprintf_xxx() functions that rely on
// android::base::System to make them mockable during unit-testing.

char* bufprint_app_dir(char* buff, char* end) {
    String appDir = System::get()->getProgramDirectory();

    if (appDir.empty()) {
        derror("Cannot locate application directory");
        exit(1);
    }

    return bufprint(buff, end, "%s", appDir.c_str());
}

char* bufprint_avd_home_path(char* buff, char* end) {
    static const char kAvdSubDir[] = "avd";
    const char* home = System::get()->envGet("ANDROID_AVD_HOME");
    if (home) {
        return bufprint(buff, end, "%s", home);
    }
    buff = bufprint_config_path(buff, end);
    return bufprint(buff, end, "%c%s", System::kDirSeparator, kAvdSubDir);
}

char* bufprint_config_path(char* buff, char* end) {
    // Name of the Android configuration directory under $HOME.
    static const char kAndroidSubDir[] = ".android";
    System* system = System::get();
    const char* home = system->envGet("ANDROID_EMULATOR_HOME");
    if (home) {
        return bufprint(buff, end, "%s", home);
    }
    home = system->envGet("ANDROID_SDK_HOME");
    if (home) {
        return bufprint(buff, end, "%s%c%s", home, System::kDirSeparator,
                        kAndroidSubDir);
    }
    String homeDir = system->getHomeDirectory();
    if (homeDir.empty()) {
        homeDir = system->getTempDir();
        if (homeDir.empty()) {
            homeDir = "/tmp";
        }
    }
    return bufprint(buff, end, "%s%c%s", homeDir.c_str(),
                    System::kDirSeparator, kAndroidSubDir);
}

char* bufprint_config_file(char* buff, char* end, const char* suffix) {
    char* p = bufprint_config_path(buff, end);
    p = bufprint(p, end, "%c%s", System::kDirSeparator, suffix);
    return p;
}

char* bufprint_temp_dir(char* buff, char* end) {
    return bufprint(buff, end, "%s", System::get()->getTempDir().c_str());
}

char* bufprint_temp_file(char* buff, char* end, const char* suffix) {
    return bufprint(buff, end, "%s%c%s", System::get()->getTempDir().c_str(),
                    System::kDirSeparator, suffix);
}
