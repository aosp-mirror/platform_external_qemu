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

#include "WifiForwardServer.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace network {

static const android::base::Looper::Duration kRetryServerStartDelayMs = 15000;

using android::base::AsyncSocketServer;
using android::base::Looper;
using android::base::ThreadLooper;

WifiForwardServer::WifiForwardServer(uint16_t port,
                                     OnDataAvailableCallback onDataAvailable)
    : WifiForwardPeer(onDataAvailable),
      mTimer(mLooper->createTimer(&WifiForwardServer::onTimeout, this)),
      mPort(port) {

}

void WifiForwardServer::start() {
    createServer();
}

void WifiForwardServer::createServer() {
    if (!mServer) {
        auto callback = [=](int fd) -> bool { return this->onConnect(fd); };
        mServer = AsyncSocketServer::createTcpLoopbackServer(mPort, callback);
    }
    if (mServer) {
        mServer->startListening();
    } else {
        // Can't create server right now, try later
        mTimer->startRelative(kRetryServerStartDelayMs);
    }
}

void WifiForwardServer::onTimeout(void* opaque, Looper::Timer* /*timer*/) {
    auto server = static_cast<WifiForwardServer*>(opaque);
    if (!server->mServer) {
        server->createServer();
    }
}

}  // namespace network
}  // namespace android

