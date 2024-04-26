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

#include <ostream>  // for operator<<, ostream
#include <string>   // for string, allocator, operator+

#include "aemu/base/files/PathUtils.h"      // for pj
#include "aemu/base/logging/Log.h"          // for LOG, LogMessage
#include "aemu/base/logging/LogSeverity.h"  // for EMULATOR_LOG_ERROR
#include "android/base/system/System.h"        // for System
#include "android/qt/qt_path.h"                // for androidQtGetLibraryDir
#include "android/utils/debug.h"               // for VERBOSE_PRINT, VERBOSE_init

using android::base::pj;

auto androidQtSetupEnv(int bitness, const char* emulatorDir) -> bool {
    // Add <progdir>/<lib>/qt/lib if it exists to the library search path.
    std::string qtLibSubDir = androidQtGetLibraryDir(bitness, emulatorDir);
    auto* system = android::base::System::get();

    if (!system->pathIsDir(qtLibSubDir)) {
        LOG(ERROR) << "Qt library not found at " << qtLibSubDir.c_str();
        return false;
    }
    VERBOSE_PRINT(init, "Adding library search path for Qt: '%s'",
                  qtLibSubDir.c_str());
    android::base::System::addLibrarySearchDir(qtLibSubDir);

    // Set the platforms plugin path too.
    std::string qtPluginsSubDir = androidQtGetPluginsDir(bitness, emulatorDir);

    VERBOSE_PRINT(init,
                  "Silencing all qWarning(); use qCWarning(...) instead: "
                  "QT_LOGGING_RULES=%s",
                  "default.warning=false");
    system->envSet("QT_LOGGING_RULES", "default.warning=false");

    VERBOSE_PRINT(
            init,
            "Setting Qt plugin search path: QT_QPA_PLATFORM_PLUGIN_PATH=%s",
            qtPluginsSubDir.c_str());
    system->envSet("QT_QPA_PLATFORM_PLUGIN_PATH", qtPluginsSubDir);

    VERBOSE_PRINT(init,
                  "Setting Qt to use software OpenGL: QT_OPENGL=software");
    system->envSet("QT_OPENGL", "software");
    VERBOSE_PRINT(init,
                  "Setting QML to use software QtQuick2D: "
                  "QMLSCENE_DEVICE=softwarecontext");
    system->envSet("QMLSCENE_DEVICE", "softwarecontext");

    VERBOSE_PRINT(init, "Overriding pre-existing bad Qt high dpi settings...");
    system->envSet("QT_AUTO_SCREEN_SCALE_FACTOR", "none");
    system->envSet("QT_SCALE_FACTOR", "none");
    system->envSet("QT_SCREEN_SCALE_FACTORS", "none");

#if defined(__linux__) || defined(__APPLE__)
    std::string qt_chromium_env = system->envGet("QTWEBENGINE_CHROMIUM_FLAGS");
    if (qt_chromium_env.empty()) {
        // We didn't build QtWebEngine with opengl support, so we need to explicitly disable
        // it in order for it to work.
        VERBOSE_PRINT(init, "Forcing QtWebEngine to use software rendering");
        if (system->envGet("ANDROID_EMU_QTWEBENGINE_DEBUG").empty()) {
            system->envSet("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");
        } else {
            const std::string kDebuggingPort = "9876";
            system->envSet("QTWEBENGINE_CHROMIUM_FLAGS",
                           std::string("--disable-gpu --webEngineArgs --remote-debugging-port=") +
                           kDebuggingPort);
            // Forward javascript console logs to js Qt Logging Category handler.
            system->envSet("QT_LOGGING_RULES", "js.debug=true;js.info=true");
            LOG(INFO) << "********* QtWebEngine debugging enabled. Point your browser to localhost:"
                      << kDebuggingPort;
        }
    } else {
        VERBOSE_PRINT(init, "Using user-provided QTWEBENGINE_CHROMIUM_FLAGS=%s",
                      qt_chromium_env.c_str());
    }
#endif  // defined(__linux__) || defined(__APPLE__)

#ifdef __linux__
    // On some linux distributions, the kernel's anonymous namespaces feature is
    // turned off, which may cause issues with QtWebEngineProcess in sandbox mode
    // (See https://doc.qt.io/qt-6/qtwebengine-platform-notes.html#sandboxing-support).
    // So let's turn off sandboxing.
    system->envSet("QTWEBENGINE_DISABLE_SANDBOX", "1");

    // Some systems will not use the right libfreetype.
    // LD_PRELOAD that stuff.
    auto freetypePath = pj(qtLibSubDir, "libfreetype.so.6");
    std::string existing = system->envGet("LD_PRELOAD");
    if (!existing.empty()) {
        existing = std::string(freetypePath) + " " + existing;
    } else {
        existing = std::string(freetypePath);
    }
    VERBOSE_PRINT(init, "Setting LD_PRELOAD to %s", existing);
    system->envSet("LD_PRELOAD", existing.c_str());
#endif

    return true;
}
