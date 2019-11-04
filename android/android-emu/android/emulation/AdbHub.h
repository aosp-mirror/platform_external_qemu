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

class AdbHub {
public:
    typedef std::function<void(int)> WakePipeFunc;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int onGuestSendData(const AndroidPipeBuffer* buffers, int numBuffers);
    int onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers);
    // Only used on load
    // Should be called when the pipe start proxying data
    int getProxyCount();
    android::base::Looper::FdWatch* mFdWatcher = nullptr;
    void setWakePipeFunc(WakePipeFunc wakePipe);
    unsigned onGuestPoll() const;
    void onHostSocketEvent(int fd,
                           unsigned events,
                           std::function<void()> onSocketClose);
    bool socketWantRead();
    bool socketWantWrite();

private:
    int sendToHost(const AndroidPipeBuffer* buffers, int numBuffers);
    int recvFromHost(AndroidPipeBuffer* buffers, int numBuffers);
    void checkRemoveProxy(std::unordered_map<int, AdbProxy*>::iterator proxy);
    void clearSendBufferLocked();
    void clearRecvBufferLocked();
    AdbProxy* onNewConnection(const apacket& requestPacket,
                              const apacket& replyPacket);
    AdbProxy* tryReuseConnection(const apacket& packet);
    void pushToSendQueue(apacket&& packet);
    void pushToRecvQueue(apacket&& packet);
    int readSocket(int fd);
    int writeSocket(int fd);
    std::unordered_map<int, apacket> mPendingConnections;
    // TODO: add locks when handling proxies.
    // Technically receiver thread and pipe thread can simultaneously
    // work on the same proxy.
    std::unordered_map<int, AdbProxy*> mProxies;
    std::unordered_map<int, AdbProxy*>::iterator mCurrentRecvProxy;
    std::unordered_map<int, AdbProxy*>::iterator mCurrentSendProxy;
    // Only used on load
    int mLoadRecvProxyId = -1;
    int mLoadSendProxyId = -1;

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
    std::unordered_map<int, std::unique_ptr<jdwp::JdwpProxy>> mJdwpProxies;
    WakePipeFunc mWakePipe = nullptr;
    emulation::apacket mCnxnPacket;
    bool mShouldReconnect = false;
};
}  // namespace emulation
}  // namespace android