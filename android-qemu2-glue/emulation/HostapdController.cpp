// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/emulation/HostapdController.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/emulation/ParameterList.h"
#include "android/network/Endian.h"
#include "android/utils/debug.h"

extern "C" {
#include "qemu/osdep.h"
#include "common/ieee802_11_defs.h"
#include "drivers/driver_virtio_wifi.h"
int run_hostapd_main(int argc,
                     const char** argv,
                     void (*on_main_loop_done)(void));
void eloop_terminate(void);
int eloop_terminated(void);
extern int wpa_debug_level;
}

#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(wifi)) \
            dprint(__VA_ARGS__); \
    } while (0)

using namespace android::base;

namespace android {
namespace qemu2 {

static HostapdController* sInstance = nullptr;

HostapdController* HostapdController::getInstance() {
    if (!sInstance) {
        sInstance = new HostapdController();
    }
    return sInstance;
}

HostapdController::HostapdController()
    : mTemplateConf(PathUtils::join(System::get()->getLauncherDirectory(),
                                    "lib",
                                    "hostapd.conf")),
      mLowestInterfaceNumber(1),
      mRunning(false),
      mEvent(std::make_unique<Event>()) {}

bool HostapdController::initAndRun(bool verbose) {
    if (mRunning)
        return false;

    mRunning = true;
    if (verbose) {
        wpa_debug_level = (int)DebugLevel::MSG_DEBUG;
    } else {
        // Disable logging default.
        wpa_debug_level = (int)DebugLevel::MSG_ERROR + 1;
    }
    return async(&enterMainLoop);
}

void HostapdController::setDriverSocket(ScopedSocket sock) {
    mSock = std::move(sock);
    ::set_virtio_sock(mSock.get());
}

bool HostapdController::addSsid(std::string ssid, std::string passphrase) {
    return true;
}

bool HostapdController::blockOnSsid(std::string ssid, bool blocked) {
    return true;
}

bool HostapdController::isBlocked(std::string ssid) {
    const auto& iter = mAccessPoints.find(ssid);
    if (iter != mAccessPoints.end()) {
        return iter->second.blocked;
    } else {
        return false;
    }
}

void HostapdController::terminate(bool wait) {
    eloop_terminate();
    if (wait) {
        if (!eloop_terminated())
            mEvent->wait();
    }
    mEvent = std::make_unique<Event>();
}
void HostapdController::enterMainLoop() {
    auto* hostapd = HostapdController::getInstance();
    if (!hostapd)
        return;
    android::ParameterList args{"hostapd"};
    auto hostapdConf = hostapd->writeConfig();
    args.add(hostapdConf);
    fprintf(stderr, "Starting hostapd main loop.");
    run_hostapd_main(args.size(), (const char**)args.array(), nullptr);
    fprintf(stderr, "Hostapd main loop has stopped.");
    hostapd->mRunning = false;
    hostapd->mEvent->signal();
}

std::string HostapdController::writeConfig() {
    return mTemplateConf;
}

}  // namespace qemu2
}  // namespace android
