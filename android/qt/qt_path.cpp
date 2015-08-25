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

#include "android/qt/qt_setup.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

using namespace android::base;

// Get the base directory for libraries and plugins.
static String androidQtGetBaseDir()
{
    System* system = System::get();
    const char* libBitness = (system->getHostBitness() == 64) ? "lib64" : "lib";
    StringVector subDirVector;
    subDirVector.push_back(system->getProgramDirectory());
    subDirVector.push_back(String(libBitness));
    subDirVector.push_back(String("qt"));
    String qtDir = PathUtils::recompose(subDirVector);

    return qtDir;
}

String androidQtGetLibraryDir()
{
    StringVector subDirVector;
    subDirVector.push_back(androidQtGetBaseDir());
    subDirVector.push_back(String("lib"));
    String qtLibDir = PathUtils::recompose(subDirVector);

    return qtLibDir;
}

String androidQtGetPluginsDir()
{
    StringVector subDirVector;
    subDirVector.push_back(androidQtGetBaseDir());
    subDirVector.push_back(String("plugins"));
    String qtPluginsDir = PathUtils::recompose(subDirVector);

    return qtPluginsDir;
}
