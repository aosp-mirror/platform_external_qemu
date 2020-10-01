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
} // extern "C"

#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(wifi)) \
            dprint(__VA_ARGS__); \
    } while (0)

using namespace android::base;

namespace android {
namespace qemu2 {

static LazyInstance<HostapdController> sInstance = LAZY_INSTANCE_INIT;

HostapdController* HostapdController::getInstance() {
    return sInstance.ptr();
}

HostapdController::HostapdController()
    : mTemplateConf(PathUtils::join(System::get()->getLauncherDirectory(),
                                    "lib",
                                    "hostapd.conf")) {}

HostapdController::~HostapdController() {
    terminate(false);
}

bool HostapdController::initAndRun(bool verbose) {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return false;
    }

    if (verbose) {
        wpa_debug_level = (int)DebugLevel::MSG_DEBUG;
    } else {
        // Disable logging by default.
        wpa_debug_level = (int)DebugLevel::MSG_DISABLE;
    }

    return async([this]() {
        android::ParameterList args{"hostapd"};
        args.add(mTemplateConf);
        D("Starting hostapd main loop.");
        run_hostapd_main(args.size(), (const char**)args.array(), nullptr);
        D("Hostapd main loop has stopped.");
        mRunning = false;
        mCvLoopExit.signal();
    });
}

void HostapdController::setDriverSocket(ScopedSocket sock) {
    mSock = std::move(sock);
    ::set_virtio_sock(mSock.get());
}

void HostapdController::terminate(bool wait) {
    if (!mRunning.load())
        return;
    eloop_terminate();
    if (wait) {
        AutoLock lock(mLock);
        mCvLoopExit.wait(&lock, [this]() {
            return eloop_terminated();
        });
    }
}

}  // namespace qemu2
}  // namespace android
