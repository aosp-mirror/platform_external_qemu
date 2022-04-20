// Copyright 2015-2016 The Android Open Source Project
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
#include "android/base/Log.h"
#include "android/qt/qt_path.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>

using android::base::pj;

bool androidQtSetupEnv(int bitness, const char* emulatorDir) {
    // Add <progdir>/<lib>/qt/lib if it exists to the library search path.
    std::string qtLibSubDir = androidQtGetLibraryDir(bitness, emulatorDir);
    auto system = android::base::System::get();

    if (!system->pathIsDir(qtLibSubDir)) {
        LOG(ERROR) << "Qt library not found at " << qtLibSubDir.c_str();
        return false;
    }
    VERBOSE_PRINT(init,
        "Adding library search path for Qt: '%s'", qtLibSubDir.c_str());
    system->addLibrarySearchDir(qtLibSubDir);

    // Set the platforms plugin path too.
    std::string qtPluginsSubDir = androidQtGetPluginsDir(bitness, emulatorDir);

    VERBOSE_PRINT(init,
        "Silencing all qWarning(); use qCWarning(...) instead: QT_LOGGING_RULES=%s",
        "default.warning=false");
    system->envSet("QT_LOGGING_RULES", "default.warning=false");

    VERBOSE_PRINT(init,
        "Setting Qt plugin search path: QT_QPA_PLATFORM_PLUGIN_PATH=%s",
        qtPluginsSubDir.c_str());
    system->envSet("QT_QPA_PLATFORM_PLUGIN_PATH", qtPluginsSubDir);

    VERBOSE_PRINT(init,
        "Setting Qt to use software OpenGL: QT_OPENGL=software");
    system->envSet("QT_OPENGL", "software");
    VERBOSE_PRINT(init,
        "Setting QML to use software QtQuick2D: QMLSCENE_DEVICE=softwarecontext");
    system->envSet("QMLSCENE_DEVICE", "softwarecontext");

    VERBOSE_PRINT(init,
        "Overriding pre-existing bad Qt high dpi settings...");
    system->envSet("QT_AUTO_SCREEN_SCALE_FACTOR", "none");
    system->envSet("QT_SCALE_FACTOR", "none");
    system->envSet("QT_SCREEN_SCALE_FACTORS", "none");

#ifdef __linux__
    // Some systems will not use the right libfreetype.
    // LD_PRELOAD that stuff.
    auto freetypePath = pj(qtLibSubDir, "libfreetype.so.6");
    std::string existing = system->envGet("LD_PRELOAD");
    if (!existing.empty()) {
        existing = std::string(freetypePath) + " " + existing;
    } else {
        existing = std::string(freetypePath);
    }
    VERBOSE_PRINT(init, "Setting LD_PRELOAD to %s", existing.c_str());
    system->envSet("LD_PRELOAD", existing.c_str());
#endif

    return true;
}
