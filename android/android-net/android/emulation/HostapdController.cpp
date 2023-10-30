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
#include "android/emulation/HostapdController.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include "aemu/base/Log.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/misc/FileUtils.h"
#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "aemu/base/threads/Async.h"
#include "android/emulation/ParameterList.h"
#include "host-common/FeatureControl.h"
#include "android/network/Endian.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#include <sstream>

extern "C" {
#include "common/ieee802_11_defs.h"
#include "drivers/driver_virtio_wifi.h"
int run_hostapd_main(int argc,
                     const char** argv,
                     void (*on_main_loop_done)(void));
void eloop_terminate(void);
extern int wpa_debug_level;
} // extern "C"

using namespace android::base;
using android::network::CipherScheme;
namespace fc = android::featurecontrol;

static std::string buildHostapdConfig(const std::string& ssid,
                                      const std::string& password) {
    std::stringstream ss;
    ss << "ssid=" << ssid << "\n";
    if (!password.empty()) {
        ss << "wpa=2\n";
        ss << "wpa_key_mgmt=WPA-PSK\n";
        ss << "rsn_pairwise=CCMP\n";
        ss << "wpa_passphrase=" << password << "\n";
    }
    ss << "interface=wlan1\n";
    ss << "driver=virtio_wifi\n";
    ss << "bssid=00:13:10:95:fe:0b\n";
    ss << "country_code=US\n";
    ss << "hw_mode=g\n";
    ss << "channel=8\n";
    ss << "beacon_int=1000\n";
    ss << "dtim_period=2\n";
    ss << "max_num_sta=255\n";
    ss << "rts_threshold=2347\n";
    ss << "fragm_threshold=2346\n";
    ss << "macaddr_acl=0\n";
    ss << "auth_algs=3\n";
    ss << "ignore_broadcast_ssid=0\n";
    ss << "wmm_enabled=0\n";
    ss << "ieee80211n=1\n";
    ss << "eapol_key_index_workaround=0\n";
    return ss.str();
}

namespace android {
namespace emulation {

const char* const HostapdController::kAndroidWifiSsid = "AndroidWifi";

static LazyInstance<HostapdController> sInstance = LAZY_INSTANCE_INIT;

HostapdController* HostapdController::getInstance() {
    return sInstance.ptr();
}

HostapdController::HostapdController()
    : mTemplateConf(PathUtils::join(System::get()->getLauncherDirectory(),
                                    "lib",
                                    "hostapd.conf")),
      mSsid(kAndroidWifiSsid) {}

HostapdController::~HostapdController() {
    terminate();
}

bool HostapdController::init(bool verbose) {
    if (mInitialized.load()) {
        return false;
    }
    if (verbose) {
        wpa_debug_level = (int)DebugLevel::MSG_DEBUG;
        mVerbose = verbose;
    } else {
        // Disable logging by default.
        wpa_debug_level = (int)DebugLevel::MSG_DISABLE;
    }
    int fds[2];
    if (socketCreatePair(&fds[0], &fds[1])) {
        derror("Unable to initialize HostapdController due to socket creation "
               "failure, err: %s",
               strerror(errno));
        return false;
    }
    socketSetNonBlocking(fds[0]);
    socketSetNonBlocking(fds[1]);
    mCtrlSock1 = ScopedSocket(fds[0]);
    mCtrlSock2 = ScopedSocket(fds[1]);
    mInitialized = true;
    return true;
}

bool HostapdController::run() {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return false;
    }
    return async([this]() {
        android::ParameterList args{"hostapd"};
        if (mVerbose) {
            args.add("-dd");
        }
        mConfigFile = genConfigFile();
        args.add(mConfigFile);
        LOG(DEBUG) << "Starting hostapd main loop.";
        run_hostapd_main(args.size(), (const char**)args.array(), nullptr);
        LOG(DEBUG) << "Hostapd main loop has stopped.";
        mRunning = false;
    });
}

bool HostapdController::setDriverSocket(android::base::ScopedSocket& sock) {
    bool res = false;
    if (isRunning()) {
        if (sock.valid() && mCtrlSock1.valid()) {
            res = !::set_virtio_sock(sock.get()) &&
                  !::set_virtio_ctrl_sock(mCtrlSock1.get());
        }
    }
    if (res) {
        swap(mDataSock, sock);
    }
    return res;
}

bool HostapdController::setSsid(std::string ssid, std::string password) {
    if (ssid.empty()) {
        LOG(DEBUG) << "Must have a non-empty SSID name";
        return false;
    }
    if (!mSsid.compare(ssid) && !mPassword.compare(password)) {
        LOG(DEBUG) << "SSID and password match with current configuration. "
                        "No need to "
                        "restart.";
        return true;
    }
    mSsid = std::move(ssid);
    mPassword = std::move(password);
    // Update the content in the config file.
    FILE* fp = android_fopen(mConfigFile.c_str(), "w");
    if (!android::writeStringToFile(fileno(fp),
                                    buildHostapdConfig(mSsid, mPassword))) {
        derror("Failed to write to config file: %s", mConfigFile);
    }
    fclose(fp);
    if (socketSend(mCtrlSock2.get(), VIRTIO_WIFI_CTRL_CMD_RELOAD_CONFIG,
                   sizeof(VIRTIO_WIFI_CTRL_CMD_RELOAD_CONFIG) - 1) < 0) {
        derror("Virtual wifi failed to send control command to hostapd.");
    }
    return true;
}

void HostapdController::terminate() {
    if (!mRunning.load())
        return;

    // Do not send null character.
    if (socketSend(mCtrlSock2.get(), VIRTIO_WIFI_CTRL_CMD_TERMINATE,
                   sizeof(VIRTIO_WIFI_CTRL_CMD_TERMINATE) - 1) < 0) {
        derror("Failed to gracefully shutdown virtual wifi, terminating. ");
        ::eloop_terminate();
    }
}

bool HostapdController::isRunning() {
    return mRunning.load();
}

bool HostapdController::usingWPA2() {
    if (!isRunning()) {
        return false;
    }
    if (mPassword.empty()) {
        return false;
    }
    const auto ptk = ::get_active_ptk();
    const auto gtk = ::get_active_gtk();
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

const std::string HostapdController::genConfigFile() {
    // Use the default template config file if ssid is "AndroidWifi"
    // and password is empty.
    if (!mSsid.compare(kAndroidWifiSsid) && mPassword.empty()) {
        return mTemplateConf;
    }
    // Otherwise, generate a hard-coded hostapd config file with the
    // newly-created SSID and password.
    // TODO(wdu@) path_copy_file() has bug on Windows.
    // Therefore, we use the hard-coded function instead.
    // Need to fix path_copy_file() instead.
    auto* tempFile = tempfile_create_with_ext(".conf");
    const char* filePath = tempfile_path(tempFile);
    FILE* fp = android_fopen(filePath, "w");
    if (!android::writeStringToFile(fileno(fp),
                                    buildHostapdConfig(mSsid, mPassword))) {
        derror("Failed to write to virtual wifi config file: %s ",
               mConfigFile.c_str());
    }
    fclose(fp);
    return filePath;
}

}  // namespace qemu2
}  // namespace android
