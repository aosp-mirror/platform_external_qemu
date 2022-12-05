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

#include <string>  // for string
#include <string_view>
#include <vector>  // for vector

#include "aemu/base/Debug.h"                 // for WaitForDebugger
#include "aemu/base/Log.h"                   // for LOG, LogMessage
#include "aemu/base/ProcessControl.h"        // for createEscapedLaunchSt...
#include "aemu/base/StringFormat.h"          // for StringAppendFormatRaw

#include "aemu/base/files/PathUtils.h"       // for PathUtils
#include "aemu/base/misc/StringUtils.h"      // for strDup
#include "android/base/system/System.h"         // for System
#include "android/cmdline-option.h"             // for android_cmdLine
#include "android/crashreport/HangDetector.h"   // for HangDetector
#include "host-common/crash-handler.h"  // for crashhandler_cleanup
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
using android::base::System;

// The order of initialization here can be very finicky. Handle with care, and
// leave hints about any ordering constraints via comments.
void process_early_setup(int argc, char** argv) {
    // This function is the first thing emulator calls - so it's the best place
    // to wait for a debugger to attach, before even the options parsing code.
    static constexpr std::string_view waitForDebuggerArg = "-wait-for-debugger";
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

    // libcurl initialization is thread-unsafe, so let's call it first
    // to make sure no other thread could be doing the same
    std::string launcherDir = System::get()->getLauncherDirectory();
    std::string caBundleFile =
            PathUtils::join(launcherDir, "lib", "ca-bundle.pem");
    if (!System::get()->pathCanRead(caBundleFile)) {
        dprint("Can not read ca-bundle. Curl init skipped.");
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
}
