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

#include <stdio.h>
#include <stdlib.h>

using namespace android::base;

bool androidQtSetupEnv() {
    // Add <progdir>/<lib>/qt if it exists to the library search path.
    String  qtLibSubDir = androidQtGetLibraryDir();
    System* system = System::get();

    if (!system->pathIsDir(qtLibSubDir.c_str())) {
        LOG(ERROR) << "Qt library not found at " << qtLibSubDir.c_str();
        return false;
    }
    //LOG(INFO) << "Adding library search path [" << qtLibSubDir.c_str() << "]";
    system->addLibrarySearchDir(qtLibSubDir.c_str());
    //const char* env = ::getenv(System::kLibrarySearchListEnvVarName);
    //LOG(INFO) << "Library search list [" << (env ? env : "<empty>") << "]";

    // Set the platforms plugin path too.
    system->envSet("QT_QPA_PLATFORM_PLUGIN_PATH", qtLibSubDir.c_str());

    return true;
}
