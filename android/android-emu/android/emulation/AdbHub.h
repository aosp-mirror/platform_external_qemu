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

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "android/base/async/Looper.h"
#include "android/base/files/Stream.h"
#include "android/emulation/AdbProxy.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/apacket_utils.h"
#include "android/jdwp/JdwpProxy.h"

namespace android {
namespace emulation {

// AdbHub is a hub to parse all incoming adb traffic from one adb pipe,
// and forward it to a proxy if the corresponding proxy is supported
// (currently only jdwp proxy is supported). On guest send message, the
// proxy can choose to forward the message to the host, or compose its
// own reply message to the guest. If no corresponding proxy implemnetaton
// is registered, the message will be simply forwarded to the host.

// AdbHub supports snapshot, and reconnecting a new host connect / open
// request to an existing guest pipe / stream on snapshot load. In another
// word, on snapshot load, it can switch host connection without
// disconnecting the guest.
class AdbHub {
public:
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int onGuestSendData(const AndroidPipeBuffer* buffers, int numBuffers);
    int onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers);
    // Only used on snapshot to figure out if we should snapshot this pipe,
    // and in unit tests.
    int getProxyCount();
    unsigned onGuestPoll() const;
    void onHostSocketEvent(int fd,
                           unsigned events,
                           std::function<void()> onSocketClose);
    bool socketWantRead();
    bool socketWantWrite();
    int pipeWakeFlags();
private:
    int sendToHost(const AndroidPipeBuffer* buffers, int numBuffers);
    int recvFromHost(AndroidPipeBuffer* buffers, int numBuffers);
    void checkRemoveProxy(std::unordered_map<int, AdbProxy*>::iterator proxy);
    AdbProxy* onNewConnection(const apacket& requestPacket,
                              const apacket& replyPacket);
    AdbProxy* tryReuseConnection(const apacket& packet);
    void pushToSendQueue(apacket&& packet);
    void pushToRecvQueue(apacket&& packet);
    int readSocket(int fd);
    int writeSocket(int fd);

    // Pending connection requests, indexed by host ID
    std::unordered_map<int, apacket> mPendingConnections;
    // Weak pointers to adb proxies, indexed by guest ID
    std::unordered_map<int, AdbProxy*> mProxies;
    // Jdwp proxies, indexed by guest PID
    std::unordered_map<int, std::unique_ptr<jdwp::JdwpProxy>> mJdwpProxies;

    std::queue<apacket> mSendToHostQueue;
    apacket mCurrentGuestSendPacket;
    size_t mCurrentGuestSendPacketPst = 0;
    apacket mCurrentHostSendPacket;
    int mCurrentHostSendPacketPst = -1;

    std::queue<apacket> mRecvFromHostQueue;
    bool mWantRecv = false;
    apacket mCurrentGuestRecvPacket;
    int mCurrentGuestRecvPacketPst = -1;
    apacket mCurrentHostRecvPacket;
    size_t mCurrentHostRecvPacketPst = 0;

    emulation::apacket mCnxnPacket;
    bool mShouldReconnect = false;
};
}  // namespace emulation
}  // namespace android