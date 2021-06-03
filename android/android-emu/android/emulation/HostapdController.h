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

#include "android/base/Compiler.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/network/Ieee80211Frame.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {
class HostapdController {
public:
    HostapdController();
    ~HostapdController();
    bool initAndRun(bool verbose);
    bool usingWPA2();
    bool isRunning();
    int getDriverSocket();
    android::network::CipherScheme getCipherScheme();
    void terminate(bool wait);

    // Currently doesn't support multiple BSS. Assume there is
    // only one ssid at a time.
    bool setSsid(const std::string ssid, const std::string password);
    static HostapdController* getInstance();

private:
    enum class DebugLevel {
        MSG_EXCESSIVE,
        MSG_MSGDUMP,
        MSG_DEBUG,
        MSG_INFO,
        MSG_WARNING,
        MSG_ERROR,
        MSG_DISABLE,
    };
    std::string mTemplateConf;
    std::atomic<bool> mRunning{false};
    // mLock is used to wait for hostapd event loop termination.
    android::base::Lock mLock;
    android::base::ConditionVariable mCvLoopExit;
    std::string mSsid;
    std::string mPassword;
    android::base::ScopedSocket mVirtioSock1;
    android::base::ScopedSocket mVirtioSock2;
    const std::string getTempConfigurationFile();
    bool writeToConfig(FILE* fp);
    DISALLOW_COPY_ASSIGN_AND_MOVE(HostapdController);
};
}  // namespace qemu2
}  // namespace android
