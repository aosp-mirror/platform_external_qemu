// Copyright 2018 The Android Open Source Project
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

#include "android/base/async/Looper.h"
#include "android/network/WifiForwardPeer.h"

#include <stdint.h>

namespace android {
namespace network {

class WifiForwardClient : public WifiForwardPeer {
public:
    WifiForwardClient(uint16_t port, OnDataAvailableCallback onDataAvailable);

private:
    void start() override;
    static void onTimeout(void* opaque, android::base::Looper::Timer* timer);

    android::base::Looper::Timer* mTimer;
    uint16_t mPort;
};

}  // namespace network
}  // namespace android

