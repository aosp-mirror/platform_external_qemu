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
#include "android/base/synchronization/Event.h"

#include <memory>
#include <string>
#include <atomic>

namespace android {
namespace qemu2 {
class HostapdController {
public:
    static HostapdController* getInstance();
    HostapdController();
    bool initAndRun(bool verbose);
    void setDriverSocket(android::base::ScopedSocket sock);
    void terminate(bool wait);

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(HostapdController);

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
    std::unique_ptr<android::base::Event> mEvent;
    android::base::ScopedSocket mSock;
};
}  // namespace qemu2
}  // namespace android
