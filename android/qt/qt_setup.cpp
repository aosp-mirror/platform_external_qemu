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

#include "android/base/system/System.h"
#include "android/base/StringFormat.h"
#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/qt/qt_path.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>

using namespace android::base;

bool androidQtSetupEnv(int bitness) {
    // Add <progdir>/<lib>/qt/lib if it exists to the library search path.
    String  qtLibSubDir = androidQtGetLibraryDir(bitness);
    System* system = System::get();

    if (!system->pathIsDir(qtLibSubDir.c_str())) {
        LOG(ERROR) << "Qt library not found at " << qtLibSubDir.c_str();
        return false;
    }
    VERBOSE_PRINT(init,
        "Adding library search path for Qt: '%s'", qtLibSubDir.c_str());
    system->addLibrarySearchDir(qtLibSubDir.c_str());

    // Set the platforms plugin path too.
    String qtPluginsSubDir = androidQtGetPluginsDir(bitness);

    VERBOSE_PRINT(init,
        "Setting Qt plugin search path: QT_QPA_PLATFORM_PLUGIN_PATH=%s",
        qtPluginsSubDir.c_str());
    system->envSet("QT_QPA_PLATFORM_PLUGIN_PATH", qtPluginsSubDir.c_str());

    return true;
}
