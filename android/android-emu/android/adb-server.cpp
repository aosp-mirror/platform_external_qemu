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

#include "aemu/base/files/StdioStream.h"
#include "aemu/base/files/Stream.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/threads/Async.h"
#include "android/base/system/System.h"
#include "android/emulation/AdbDebugPipe.h"
#include "android/emulation/AdbGuestPipe.h"
#include "android/emulation/AdbVsockPipe.h"
#include "android/emulation/AdbHostListener.h"
#include "android/emulation/AdbHostServer.h"
#include "host-common/AndroidPipe.h"
#include "android/utils/debug.h"

#include "host-common/feature_control.h"

#include <memory>
#include <string>

#include <stdio.h>
#include <stdlib.h>

namespace {

using android::emulation::AdbGuestAgent;
using android::emulation::AdbGuestPipe;
using android::emulation::AdbVsockPipe;
using android::emulation::AdbHostListener;
using android::emulation::AdbHostServer;
using android::AndroidPipe;

// Global variables used here.
struct Globals {
    Globals() : hostListener(AdbHostServer::getClientPort()) {}

    // Register adb pipe service.
    void registerServices() {
        if (feature_is_enabled(kFeature_VirtioVsockPipe)) {
            auto service = new AdbVsockPipe::Service(&hostListener);
            hostListener.setGuestAgent(service);
            adbGuestAgent = service;
        } else {
            auto service = new AdbGuestPipe::Service(&hostListener);
            hostListener.setGuestAgent(service);
            AndroidPipe::Service::add(std::unique_ptr<AdbGuestPipe::Service>(service));
            adbGuestAgent = service;
        }

        // Register adb-debug pipe service.
        android::base::Stream* debugStream = nullptr;
        if (VERBOSE_CHECK(adb)) {
            debugStream = new android::base::StdioStream(stderr);
        }
        AndroidPipe::Service::add(
            std::make_unique<android::emulation::AdbDebugPipe::Service>(
                debugStream));
    }

    AdbGuestAgent* adbGuestAgent = nullptr;
    AdbHostListener hostListener;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

}  // namespace

void android_adb_server_notify(int port) {
    auto globals = sGlobals.ptr();
    globals->hostListener.reset(port);

    // Nothing depends on the result of this operation, but it could potentially
    // take seconds if ipv6 loopback is down (e.g. on Windows).
    // Let's run it asynchronously to not delay the startup.
    android::base::async([globals] {
        globals->hostListener.notifyServer();
    });
}

int android_adb_server_init(int port) {
    auto globals = sGlobals.ptr();
    if (!globals->hostListener.reset(port)) {
        return -1;
    }
    return 0;
}

int android_get_jdwp_port() {
    return sGlobals->hostListener.jdwpPort();
}

void android_adb_server_undo_init(void) {
    sGlobals->hostListener.reset(-1);
}

void android_adb_service_init(void) {
    // Register adb pipe service.
    sGlobals->registerServices();
}

void android_adb_reset_connection(void) {
    if (sGlobals->adbGuestAgent) {
        sGlobals->adbGuestAgent->resetActiveGuestPipeConnection();
    }
}
