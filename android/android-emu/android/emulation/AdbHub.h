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
#include <vector>
#include <unordered_map>

#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/files/Stream.h"
#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/emulation/AdbProxy.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/apacket_utils.h"
#include "android/emulation/AdbMessageSniffer.h"
#include "android/emulation/CrossSessionSocket.h"

namespace android {
namespace jdwp {
    class JdwpProxy;
}

namespace emulation {


class AdbHub {
public:
    ~AdbHub();
    typedef std::function<void(int)> WakePipeFunc;
    struct Buffer {
        std::vector<char> mBuffer = std::vector<char>(50);
        size_t mBufferTail = 0;
        size_t mBytesToSkip = 0;
        void readBytes(const AndroidPipeBuffer* buffers,
                       int numBuffers,
                       int* bufferIdx,       // input / output
                       size_t* bufferPst,    // input / output
                       size_t* remainBytes,  // input / output
                       size_t bytesToRead,
                       bool skipData,
                       std::vector<uint8_t>* clonedData);
        void onSave(android::base::Stream* stream);
        void onLoad(android::base::Stream* stream);
    };
    Buffer mGuestSendBuffer;
    Buffer mGuestRecvBuffer;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int onGuestSendData(const AndroidPipeBuffer* buffers,
                         int numBuffers);
    int onGuestRecvData(AndroidPipeBuffer* buffers,
                         int numBuffers);
    // Only used on load
    // Should be called when the pipe start proxying data
    void setSocket(CrossSessionSocket* socket);
    bool needTranslate() const;
    int getProxyCount();
    android::base::Looper::FdWatch* mFdWatcher = nullptr;
    void setWakePipeFunc(WakePipeFunc wakePipe);
    unsigned onGuestPoll() const;
    void onHostSocketEvent(int fd, unsigned events, std::function<void()> onSocketClose);
    bool socketWantRead();
    bool socketWantWrite();
private:
    int sendToHost(const AndroidPipeBuffer* buffers,
                                  int numBuffers);
    int recvFromHost(AndroidPipeBuffer* buffers, int numBuffers);
    void checkRemoveProxy(std::unordered_map<int, AdbProxy*>::iterator proxy);
    void clearSendBufferLocked();
    void clearRecvBufferLocked();
    AdbProxy* onNewConnection(const apacket &requestPacket, const apacket &replyPacket);
    AdbProxy* tryReuseConnection(const apacket &packet);
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
    base::Lock mHandlePacketLock;
    
    CrossSessionSocket* mSocket = nullptr;
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
    bool mNeedTranslate = false;
    std::unordered_map<int, std::unique_ptr<jdwp::JdwpProxy>> mJdwpProxies;
    jdwp::JdwpProxy* mReconnectingProxy = nullptr;
    WakePipeFunc mWakePipe = nullptr;
    std::set<unsigned int> mIgnoreHostIds;
    bool mShouldForwardSendMessage = true;
    emulation::apacket mCnxnPacket;
    bool mShouldReconnect = false;
};
}  // namespace jdwp
}  // namespace android