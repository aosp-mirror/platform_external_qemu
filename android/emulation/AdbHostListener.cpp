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

#include "android/emulation/AdbHostListener.h"

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/emulation/AdbTypes.h"

#include <memory>
#include <vector>

namespace android {
namespace emulation {

using namespace android::base;

bool AdbHostListener::init(int adbPort) {
    CHECK(adbPort >= 0 && adbPort < 65536);
    mServer = AsyncSocketServer::createTcpLoopbackServer(
            adbPort,
            [this](int port) { return this->onHostServerConnection(port); },
            AsyncSocketServer::kIPv4AndOptionalIPv6,
            android::base::ThreadLooper::get());
    if (!mServer) {
        return false;
    }
    // Don't start listening until there is an actual active guest pipe.
    return true;
}

void AdbHostListener::deinit(void) {
    mServer.reset();
    mActiveGuest = nullptr;
    mGuests.clear();
}

void AdbHostListener::addGuest(AdbGuestAgent* guest) {
    mGuests.emplace_back(guest);
    if (!mActiveGuest && mGuests.size() > 0) {
        // As soon as we have a guest pipe, start listening.
        mServer->startListening();
    }
}

void AdbHostListener::deleteGuest(AdbGuestAgent* guest) {
    if (guest == mActiveGuest) {
        mActiveGuest = nullptr;
        if (mGuests.size() > 1) {
            mServer->startListening();
        }
    }
    for (auto it = mGuests.begin(); it != mGuests.end(); ++it) {
        if (it->get() == guest) {
            mGuests.erase(it);
            break;
        }
    }
}

bool AdbHostListener::notifyAdbServer(int adbClientPort, int adbPort) {
    // First connect to ADB server.
    ScopedSocket socket(android::base::socketTcp4LoopbackClient(adbClientPort));
    if (!socket.valid()) {
        socket.reset(android::base::socketTcp6LoopbackClient(adbClientPort));
    }
    if (!socket.valid()) {
        return false;
    }
    // Send the special message
    auto message = StringFormat("host:emulator:%d", adbPort);
    auto handshake = StringFormat("%04x%s", message.size(), message.c_str());
    LOG(VERBOSE) << "Attempting to notify adb host server with |" << handshake
                 << "|";
    return android::base::socketSendAll(socket.get(), handshake.c_str(),
                                        handshake.size());
}

bool AdbHostListener::onHostServerConnection(int socket) {
    CHECK(mActiveGuest == nullptr);
    CHECK(mGuests.size() > 0);
    mActiveGuest = mGuests[0].get();
    if (!mActiveGuest->onHostConnection(socket)) {
        deleteGuest(mActiveGuest);
        return false;
    } else {
        if (mAdbClientPort) {
            notifyAdbServer(mAdbClientPort, mServer->port());
        }
        return true;
    }
}

}  // namespace emulation
}  // namespace android
