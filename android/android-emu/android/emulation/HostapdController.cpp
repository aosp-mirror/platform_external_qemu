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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android/emulation/HostapdController.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/ParameterList.h"
#include "android/network/Endian.h"
#include "android/utils/debug.h"
#include "android/utils/tempfile.h"

extern "C" {
//#include "qemu/osdep.h"
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
using android::network::CipherScheme;

namespace android {
namespace emulation {

static LazyInstance<HostapdController> sInstance = LAZY_INSTANCE_INIT;

HostapdController* HostapdController::getInstance() {
    return sInstance.ptr();
}

HostapdController::HostapdController()
    : mTemplateConf(PathUtils::join(System::get()->getLauncherDirectory(),
                                    "lib",
                                    "hostapd.conf")),
      mSsid("AndroidWifi") {
    int fds[2];
    if (socketCreatePair(&fds[0], &fds[1])) {
        LOG(ERROR) << "HostapdController: Unable to create socket pair";
    }
    mVirtioSock1 = ScopedSocket(fds[0]);
    mVirtioSock2 = ScopedSocket(fds[1]);
}

HostapdController::~HostapdController() {
    terminate(false);
}

int HostapdController::getDriverSocket() {
    return mVirtioSock2.get();
}

bool HostapdController::initAndRun(bool verbose) {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return false;
    }

    if (verbose) {
        wpa_debug_level = (int)DebugLevel::MSG_EXCESSIVE;
    } else {
        // Disable logging by default.
        wpa_debug_level = (int)DebugLevel::MSG_DISABLE;
    }
    // set io socket before event loop starts.
    ::set_virtio_sock(mVirtioSock1.get());
    return async([this]() {
        android::ParameterList args{"hostapd"};
        args.add(getTempConfigurationFile());
        D("Starting hostapd main loop.");
        run_hostapd_main(args.size(), (const char**)args.array(), nullptr);
        D("Hostapd main loop has stopped.");
        mRunning = false;
        mCvLoopExit.signal();
    });
}

void HostapdController::terminate(bool wait) {
    if (!isRunning())
        return;
    ::eloop_terminate();

    if (wait) {
        AutoLock lock(mLock);
        mCvLoopExit.wait(&lock, [this]() { return !mRunning.load(); });
    }
}

bool HostapdController::isRunning() {
    return mRunning.load();
}

bool HostapdController::usingWPA2() {
    if (!isRunning()) {
        return false;
    }
    struct key_data ptk = ::get_active_ptk();
    struct key_data gtk = ::get_active_gtk();
    return ptk.key_len != 0 && gtk.key_len != 0;
}

android::network::CipherScheme HostapdController::getCipherScheme() {
    if (usingWPA2()) {
        // TODO(wdu@) Add support for more than CCMP cipher
        return CipherScheme::CCMP;
    } else {
        return CipherScheme::NONE;
    }
}

bool HostapdController::setSsid(const std::string ssid,
                                const std::string password) {
    if (ssid.empty()) {
        D("Hostapd: Must have a non-empty SSID name");
        return false;
    }
    if (mSsid.compare(ssid) == 0 && mPassword.compare(password) == 0) {
        D("SSID and password match with current configuration. No need to "
          "restart.");
        return true;
    }
    terminate(true);
    mSsid = std::move(ssid);
    mPassword = std::move(password);
    int res = initAndRun(VERBOSE_CHECK(wifi));
    return res;
}

const std::string HostapdController::getTempConfigurationFile() {
    auto* tempFile = tempfile_create_with_ext(".conf");
    const char* filePath = tempfile_path(tempFile);
    FILE* fp = fopen(filePath, "w");
    if (!writeToConfig(fp)) {
        D("Hostapd: faile to write SSID configuration to file");
    }
    fclose(fp);
    return filePath;
}

bool HostapdController::writeToConfig(FILE* fp) {
    auto str = android::readFileIntoString(mTemplateConf);
    bool ret = android::writeStringToFile(fileno(fp), str.value());
    fprintf(fp, "\n\n");
    fprintf(fp, "ssid=%s\n", mSsid.c_str());
    if (!mPassword.empty()) {
        fprintf(fp, "wpa=2\n");
        fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
        fprintf(fp, "rsn_pairwise=CCMP\n");
        fprintf(fp, "wpa_passphrase=%s\n", mPassword.c_str());
    }
    fprintf(fp, "\n");
    return ret;
}

}  // namespace qemu2
}  // namespace android
