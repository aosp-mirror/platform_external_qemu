// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/process_setup.h"

#include <string>                               // for string
#include <vector>                               // for vector

#include "android/base/Debug.h"                 // for WaitForDebugger
#include "android/base/Log.h"                   // for LOG, LogMessage
#include "android/base/ProcessControl.h"        // for createEscapedLaunchSt...
#include "android/base/StringFormat.h"          // for StringAppendFormatRaw
#include "android/base/StringView.h"            // for StringView, operator==
#include "android/base/files/PathUtils.h"       // for PathUtils
#include "android/base/misc/StringUtils.h"      // for strDup
#include "android/base/system/System.h"         // for System
#include "android/cmdline-option.h"             // for android_cmdLine
#include "android/crashreport/CrashReporter.h"  // for CrashReporter
#include "android/crashreport/HangDetector.h"   // for HangDetector
#include "android/crashreport/crash-handler.h"  // for crashhandler_cleanup
#include "android/curl-support.h"               // for curl_cleanup, curl_init
#include "android/protobuf/ProtobufLogging.h"   // for initProtobufLogger
#include "android/utils/debug.h"                // for dprint
#include "android/utils/filelock.h"             // for filelock_init
#include "android/utils/sockets.h"              // for android_socket_init

namespace android {
class ParameterList;
}  // namespace android

using android::ParameterList;
using android::base::PathUtils;
using android::base::StringAppendFormatRaw;
using android::base::StringView;
using android::base::System;

// The order of initialization here can be very finicky. Handle with care, and
// leave hints about any ordering constraints via comments.
void process_early_setup(int argc, char** argv) {
    // This function is the first thing emulator calls - so it's the best place
    // to wait for a debugger to attach, before even the options parsing code.
    static constexpr StringView waitForDebuggerArg = "-wait-for-debugger";
    for (int i = 1; i < argc; ++i) {
        if (waitForDebuggerArg == argv[i]) {
            dprint("Waiting for a debugger...");
            android::base::WaitForDebugger();
            dprint("Debugger has attached, resuming");
            break;
        }
    }

    // Initialize sockets first so curl/crash processor can use sockets.
    // Does not create any threads.
    android_socket_init();

    filelock_init();

    // ParameterList params(argc, argv);
    android_cmdLine = android::base::strDup(
            android::base::createEscapedLaunchString(argc, argv));

    // Catch crashes in everything.
    // This promises to not launch any threads...
    if (crashhandler_init()) {
        std::string arguments = "===== Command-line arguments =====\n";
        for (int i = 0; i < argc; ++i) {
            arguments += argv[i];
            arguments += '\n';
        }

        arguments += "\n===== Environment =====\n";
        const auto allEnv = System::get()->envGetAll();
        for (const std::string& env : allEnv) {
            arguments += env;
            arguments += '\n';
        }

        android::crashreport::CrashReporter::get()->attachData(
                "command-line-and-environment.txt", arguments);

        // Make sure we don't report any hangs until all related loopers
        // actually get started.
        android::crashreport::CrashReporter::get()->hangDetector().pause(true);
    } else {
        LOG(VERBOSE) << "Crash handling not initialized";
    }

    // libcurl initialization is thread-unsafe, so let's call it first
    // to make sure no other thread could be doing the same
    std::string launcherDir = System::get()->getLauncherDirectory();
    std::string caBundleFile =
            PathUtils::join(launcherDir, "lib", "ca-bundle.pem");
    if (!System::get()->pathCanRead(caBundleFile)) {
        LOG(VERBOSE) << "Can not read ca-bundle. Curl init skipped.";
    } else {
        curl_init(caBundleFile.c_str());
    }

    android::protobuf::initProtobufLogger();

    // For macOS, app nap can cause all sorts of issues with timers.
    // Disable it.
    // bug: 153616209 Not good when we want to allow sleep.
    // System::get()->disableAppNap();
}

void process_late_teardown() {
    curl_cleanup();
    crashhandler_cleanup();
}
