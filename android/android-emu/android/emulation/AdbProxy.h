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

#include <queue>

#include "android/emulation/apacket_utils.h"

namespace android {
namespace emulation {

class AdbProxy {
public:
    virtual void onGuestRecvData(const android::emulation::amessage* mesg,
                                 const uint8_t* data,
                                 bool* shouldForwardRecv,
                                 std::queue<apacket>* toSends) = 0;
    virtual void onGuestSendData(const android::emulation::amessage* mesg,
                                 const uint8_t* data) = 0;
    virtual bool shouldClose() const = 0;
    virtual int32_t guestId() const = 0;
    virtual int32_t originHostId() const = 0;
    virtual int32_t currentHostId() const = 0;
};
}  // namespace emulation
}  // namespace android
