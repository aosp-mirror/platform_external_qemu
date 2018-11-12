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

#include "WifiForwardClient.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace network {

static const android::base::Looper::Duration kRetryClientConnectDelay = 5000;

WifiForwardClient::WifiForwardClient(uint16_t port,
                                     OnDataAvailableCallback onDataAvailable)
    : WifiForwardPeer(onDataAvailable),
      mTimer(mLooper->createTimer(&WifiForwardClient::onTimeout, this)),
      mPort(port) {
}

void WifiForwardClient::start() {
    int socket = android::base::socketTcp4LoopbackClient(mPort);
    if (socket == -1) {
        socket = android::base::socketTcp6LoopbackClient(mPort);
        if (socket == -1) {
            // Failed to connect, retry again later
            mTimer->startRelative(kRetryClientConnectDelay);
            return;
        }
    }
    android::base::socketSetNonBlocking(socket);
    onConnect(socket);
}

void WifiForwardClient::onTimeout(void* opaque,
                                  android::base::Looper::Timer* /*timer*/) {
    auto client = static_cast<WifiForwardClient*>(opaque);
    client->start();
}

}  // namespace network
}  // namespace android

