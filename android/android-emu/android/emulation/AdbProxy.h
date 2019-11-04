// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/emulation/AdbMessageSniffer.h"

namespace android {
namespace emulation {

class AdbProxy {
public:
    virtual bool needProxyRecvData(const android::emulation::AdbMessageSniffer::amessage* mesg) = 0;
    virtual void onGuestRecvData(const char* data, size_t len) = 0;
    virtual bool needProxySendData(const android::emulation::AdbMessageSniffer::amessage* mesg) = 0;
    virtual void onGuestSendData(const char* data, size_t len) = 0;
    virtual bool shouldClose() const = 0;
    virtual int32_t guestId() const = 0;
    virtual int32_t originHostId() const = 0;
    virtual int32_t currentHostId() const = 0;
};
}
}
