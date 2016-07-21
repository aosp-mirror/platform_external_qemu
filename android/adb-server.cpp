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

#include "android/adb-server.h"

#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/AdbDebugPipe.h"
#include "android/emulation/AdbGuestPipe.h"
#include "android/emulation/AdbHostListener.h"
#include "android/emulation/AndroidPipe.h"
#include "android/utils/debug.h"

#include <memory>
#include <string>

#include <stdio.h>
#include <stdlib.h>

using namespace android::base;
using namespace android::emulation;

// Global variables used here.
struct Globals {
    Globals() : hostListener(getClientPort()) {}

    static int getClientPort() {
        int result = 5037;
        auto env = System::get()->envGet("ANDROID_ADB_SERVER_PORT");
        if (!env.empty()) {
            result = static_cast<int>(strtol(env.c_str(), NULL, 10));
        }
        return result;
    }

    AdbHostListener hostListener;
};

LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

void android_adb_server_notify(int port) {
    auto& listener = sGlobals->hostListener;
    listener.notifyAdbServer(listener.clientPort(), port);
}

int android_adb_server_init(int port) {
    if (!sGlobals->hostListener.init(port)) {
        return -1;
    }
    return 0;
}

void android_adb_server_undo_init(void) {
    sGlobals->hostListener.deinit();
}

void android_adb_service_init(void) {
    // Register adb pipe service.
    android::AndroidPipe::Service::add(new AdbGuestPipe::Service(
            &sGlobals->hostListener));

    // Register adb-debug pipe service.
    Stream* debugStream = nullptr;
    if (VERBOSE_CHECK(adb)) {
        debugStream = new StdioStream(stderr);
    }
    android::AndroidPipe::Service::add(new AdbDebugPipe::Service(debugStream));
}
