// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/qt/qt_path.h"

#include <algorithm>                       // for max
#include <string>                          // for string, basic_string, allo...
#include <vector>                          // for vector

#include "aemu/base/Log.h"              // for base
#include "aemu/base/files/PathUtils.h"  // for PathUtils
#include "android/base/system/System.h"    // for System

using namespace android::base;

// Get the base directory for libraries and plugins.
static auto androidQtGetBaseDir(int bitness, const char* emulatorDir)
        -> std::string {
    std::string qtBaseDir =
            android::base::System::get()->envGet("ANDROID_QT_LIB_PATH");
    if (!qtBaseDir.empty()) {
        return qtBaseDir;
    }

    if (bitness == 0) {
        bitness = System::getProgramBitness();
    }

    System* system = System::get();
    const char* const libBitness = bitness == 64 ? "lib64" : "lib";
    std::vector<std::string> subDirVector;
    subDirVector.push_back(emulatorDir != nullptr
                                   ? emulatorDir
                                   : system->getLauncherDirectory());
    subDirVector.emplace_back(libBitness);
    subDirVector.emplace_back("qt");
    std::string qtDir = PathUtils::recompose(subDirVector);

    return qtDir;
}

auto androidQtGetLibraryDir(int bitness, const char* emulatorDir)
        -> std::string {
    std::vector<std::string> subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness, emulatorDir));
    subDirVector.emplace_back("lib");
    std::string qtLibDir = PathUtils::recompose(subDirVector);

    return qtLibDir;
}

auto androidQtGetPluginsDir(int bitness, const char* emulatorDir)
        -> std::string {
    System* system = System::get();
    std::string qtPluginsDir =
            system->envGet("ANDROID_QT_QPA_PLATFORM_PLUGIN_PATH");
    if (!qtPluginsDir.empty()) {
        return qtPluginsDir;
    }

    std::vector<std::string> subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness, emulatorDir));
    subDirVector.emplace_back("plugins");
    qtPluginsDir = PathUtils::recompose(subDirVector);

    return qtPluginsDir;
}
