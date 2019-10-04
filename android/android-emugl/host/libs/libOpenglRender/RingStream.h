// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "OpenglRender/IOStream.h"
#include "OpenglRender/RenderChannel.h"

#include "android/base/ring_buffer.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"

#include <functional>
#include <vector>

namespace emugl {

// An IOStream instance that can be used by the host RenderThread to process
// messages from a pair of ring buffers (to host and from host).  It also takes
// a callback that does something when there are no available bytes to read in
// the "to host" ring buffer.
class RingStream final : public IOStream {
public:
    using OnUnavailableReadCallback = std::function<int()>;
    using GetPtrAndSizeCallback =
        std::function<void(uint64_t, char**, size_t*)>;

    RingStream(
        struct ring_buffer_with_view toHost,
        struct ring_buffer_with_view fromHost,
        OnUnavailableReadCallback onUnavailableReadFunc,
        size_t bufsize);

    RingStream(
        struct ring_buffer_with_view toHost,
        struct ring_buffer_with_view fromHost,
        struct ring_buffer_with_view toHostData,
        OnUnavailableReadCallback onUnavailableReadFunc,
        GetPtrAndSizeCallback getPtrAndSizeFunc,
        size_t bufsize);

    ~RingStream();

    int writeFully(const void* buf, size_t len) override;
    const unsigned char *readFully( void *buf, size_t len) override;

    void setRenderingActive(bool active) {
        mRenderingActive = active;
    }

    void printStats();

    enum Mode {
        Default = 0,
        PtrsWithData = 1,
        PtrsWithDataSameThread = 2,
    };

    void startAssist();
    void endAssist();

protected:
    virtual void* allocBuffer(size_t minSize) override final;
    virtual int commitBuffer(size_t size) override final;
    virtual const unsigned char* readRaw(void* buf, size_t* inout_len)
            override final;
    virtual void* getDmaForReading(uint64_t guest_paddr) override final;
    virtual void unlockDma(uint64_t guest_paddr) override final;

    const unsigned char* readRaw_sameThread(void* buf, size_t* inout_len);
    const unsigned char* readRaw_differentThread(void* buf, size_t* inout_len);

    void onSave(android::base::Stream* stream) override;
    unsigned char* onLoad(android::base::Stream* stream) override;

    void threadFunc();
    void threadFuncAssist();
    uint64_t currUs();

    struct ring_buffer_with_view mToHost;
    struct ring_buffer_with_view mFromHost;
    struct ring_buffer_with_view mToHostData;
    OnUnavailableReadCallback mOnUnavailableReadFunc;
    GetPtrAndSizeCallback mGetPtrAndSizeFunc;
    RenderChannel::Buffer mReadBuffer;
    RenderChannel::Buffer mWriteBuffer;
    size_t mReadBufferLeft = 0;
    size_t mReadBufferWriteStart = 0;
    size_t mXmits = 0;
    size_t mTotalRecv = 0;

    Mode mMode = Default;
    bool mAssistThreadEnabled = false;
    android::base::Lock mLock;
    android::base::BufferQueue<RenderChannel::Buffer> mFromGuest;
    android::base::FunctorThread mFromGuestReadingThread;
    bool mRenderingActive = false;

    struct Xfer {
        char* ptr;
        size_t size;
    };

    std::vector<uint64_t> mOutstandingXfers;
    std::vector<Xfer> mOutstandingXfersDecoded;

    uint64_t m_bufPushTimeUs = 0;
    uint64_t m_bufPopTimeUs = 0;
    uint64_t m_bufCopyOutTimeUs = 0;
    uint64_t m_bufUnavailableTimeUs = 0;
    uint64_t m_bufUnavailableSleepTimeUs = 0;

    enum AssistThreadMessage {
        Start = 0,
        Stop = 1,
        Exit = 2,
    };

    uint64_t m_assistBufPushTimeUs = 0;
    uint64_t m_startAssistTimeUs = 0;
    uint64_t m_endAssistTimeUs = 0;
    android::base::MessageChannel<AssistThreadMessage, 4> mToAssistThreadMessages;
    android::base::MessageChannel<AssistThreadMessage, 1> mFromAssistThreadMessages;
    bool mAssistThreadStopped = false;
    bool mBenchmarkEnabled = false;
};

}  // namespace emugl
