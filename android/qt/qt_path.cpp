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

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

using namespace android::base;

// Get the base directory for libraries and plugins.
static String androidQtGetBaseDir(int bitness)
{
    if (!bitness) {
        bitness = System::getProgramBitness();
    }

    System* system = System::get();
    const char* const libBitness = bitness == 64 ? "lib64" : "lib";
    StringVector subDirVector;
    subDirVector.push_back(system->getLauncherDirectory());
    subDirVector.push_back(String(libBitness));
    subDirVector.push_back(String("qt"));
    String qtDir = PathUtils::recompose(subDirVector);

    return qtDir;
}

String androidQtGetLibraryDir(int bitness)
{
    StringVector subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness));
    subDirVector.push_back(String("lib"));
    String qtLibDir = PathUtils::recompose(subDirVector);

    return qtLibDir;
}

String androidQtGetPluginsDir(int bitness)
{
    StringVector subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness));
    subDirVector.push_back(String("plugins"));
    String qtPluginsDir = PathUtils::recompose(subDirVector);

    return qtPluginsDir;
}
