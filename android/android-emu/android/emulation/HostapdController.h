/* Copyright 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include "aemu/base/Compiler.h"
#include "aemu/base/sockets/ScopedSocket.h"
#include "android/network/Ieee80211Frame.h"

#include <atomic>
#include <memory>
#include <string>

namespace android {
namespace emulation {
class HostapdController {
public:
    HostapdController();
    ~HostapdController();
    bool init(bool verbose);
    bool run();
    // Assuming hostapd event loop is already running.
    bool setDriverSocket(android::base::ScopedSocket sock);
    // Return false if ssid is empty. If [ssid] and [password] match
    // with current configurations, do not update the config file.
    // Otherwise, update the config file and reload config in hostapd.
    bool setSsid(const std::string ssid, const std::string password);
    // Non-blocking.
    void terminate();
    bool isRunning();
    bool usingWPA2();
    std::string getSsid() const {
        return mSsid;
    };
    android::network::CipherScheme getCipherScheme();
    static HostapdController* getInstance();

private:
    static const char* const kAndroidWifiSsid;
    enum class DebugLevel {
        MSG_EXCESSIVE,
        MSG_MSGDUMP,
        MSG_DEBUG,
        MSG_INFO,
        MSG_WARNING,
        MSG_ERROR,
        MSG_DISABLE,
    };
    const std::string genConfigFile();
    const std::string mTemplateConf;
    std::string mConfigFile;
    std::atomic<bool> mInitialized{false};
    std::atomic<bool> mRunning{false};
    std::string mSsid;
    std::string mPassword;
    bool mVerbose;
    android::base::ScopedSocket mDataSock;
    android::base::ScopedSocket mCtrlSock1;
    android::base::ScopedSocket mCtrlSock2;
    DISALLOW_COPY_ASSIGN_AND_MOVE(HostapdController);
};
}  // namespace qemu2
}  // namespace android
