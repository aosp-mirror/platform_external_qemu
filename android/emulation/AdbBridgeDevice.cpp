// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbBridgeDevice.h"

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"

#include <algorithm>

namespace android {
namespace emulation {

using android::base::AsyncSocketServer;
using android::base::StringFormat;
using std::unique_ptr;

// static
const size_t AdbBridgeDevice::kMaxAdbPipes = 4;

bool AdbBridgeDevice::init(int bridgePort) {
    mServer = std::move(AsyncSocketServer::createTcpLoopbackServer(
            bridgePort,
            [this](int port) { return this->onHostServerConnected(port); },
            AsyncSocketServer::kIPv4AndOptionalIPv6, mLooper));
    if (!mServer) {
        return false;
    }
    mServer->startListening();

    return notifyHostServer();
}

void AdbBridgeDevice::deInit() {
    mServer.reset();
    // Create and register AndroidPipeFuncs.
}

IAdbPipe* AdbBridgeDevice::pipeInit(void* hwPipe, const char*) {
    if (mAdbPipes.size() >= kMaxAdbPipes) {
        LOG(WARNING) << "Can not handle more open ADB pipes";
        return nullptr;
    }

    unique_ptr<IAdbPipe> pipe(mAdbPipeFactory->create(this, mLooper, hwPipe));
    if (!pipe || !pipe->init()) {
        return nullptr;
    }

    auto rawPtr = pipe.get();
    // Pass ownership.
    mAdbPipes.push_back(std::move(pipe));
    return rawPtr;
}

bool AdbBridgeDevice::onHostServerConnected(int port) {
    // Just try all our pipes and go with the first one that's willing to take
    // this port.
    for (auto& adbPipe : mAdbPipes) {
        if (adbPipe->connectTo(port)) {
            return true;
        }
    }
    return false;
}

void AdbBridgeDevice::notifyPipeDisconnection(IAdbPipe* pipe) {
    // Can't use container's |find| method because we don't want to create
    // another unique_ptr |pipe|.
    auto it = std::find_if(mAdbPipes.begin(), mAdbPipes.end(),
                           [pipe](const unique_ptr<IAdbPipe>& key) {
                               return pipe == key.get();
                           });
    if (it != mAdbPipes.end()) {
        mAdbPipes.erase(it);
    }

    // We stop listening for new connections for the Adb host server when we
    // accept one connection. Now that the connection has been broken, try to
    // reconect.
    if (mServer) {
        mServer->startListening();
        notifyHostServer();
    }
}

bool AdbBridgeDevice::notifyHostServer() {
    if (!mServer) {
        // Don't lie to adb host. We're not even listening for incoming
        // connections.
        return false;
    }

    // TODO(pprabhu) What about IPv6?
    auto fd = android::base::socketTcp4LoopbackClient(mAdbServerPort);
    if (fd < 0) {
        LOG(ERROR) << "Failed to open connection with ADB host server on port "
                   << mAdbServerPort;
        return false;
    }
    android::base::ScopedSocket scopedFd(fd);

    auto message = StringFormat("host:emulator:%d", mServer->port());
    auto handshake = StringFormat("%04x%s", message.size(), message.c_str());
    LOG(VERBOSE) << "Attempting to notify adb host server with |" << handshake
                 << "|";
    return android::base::socketSendAll(fd, handshake.c_str(),
                                        handshake.size());
}

}  // namespace emulation
}  // namespace android
