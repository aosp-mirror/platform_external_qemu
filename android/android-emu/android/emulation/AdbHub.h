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

#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>

#include "android/base/files/Stream.h"
#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/apacket_utils.h"
#include "android/emulation/AdbMessageSniffer.h"
#include "android/emulation/CrossSessionSocket.h"

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
};


class AdbHub {
public:
    typedef std::function<AdbProxy*(const apacket&, int)> OnNewConnection;
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
                       bool skipData);
        void onSave(android::base::Stream* stream);
        void onLoad(android::base::Stream* stream);
    };
    Buffer mGuestSendBuffer;
    Buffer mGuestRecvBuffer;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int onGuestSendData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         OnNewConnection onNewConnection);
    void onGuestRecvData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualRecvBytes,
                         OnNewConnection onNewConnection);
    // Only used on load
    void insertProxy(AdbProxy* proxy);
    void doneInsertProxy();
    void setSocket(CrossSessionSocket* socket);
private:
    std::unordered_map<int, apacket> mPendingConnections;
    std::unordered_map<int, AdbProxy*> mProxies;
    std::unordered_map<int, AdbProxy*>::iterator mCurrentRecvProxy;
    std::unordered_map<int, AdbProxy*>::iterator mCurrentSendProxy;
    // Only used on load
    int mLoadRecvProxyId = -1;
    int mLoadSendProxyId = -1;
    
    void checkCloseProxy(std::unordered_map<int, AdbProxy*>::iterator proxy);
    CrossSessionSocket* mSocket = nullptr;
    std::unique_ptr<base::Thread> mSendToHostThread;
    base::Lock mSendToHostLock;
    base::ConditionVariable mSendToHostCV;
    int sendToHost(const AndroidPipeBuffer* buffers,
                                  int numBuffers);
    bool mNeedTrasnalte = false;
};
}  // namespace jdwp
}  // namespace android