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
#include "android/emulation/address_space_graphics_types.h"

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
        struct asg_context context,
        android::emulation::asg::ConsumerCallbacks callbacks,
        size_t bufsize);
    ~RingStream();

    int getNeededFreeTailSize() const;

    int writeFully(const void* buf, size_t len) override;
    const unsigned char *readFully( void *buf, size_t len) override;

    void printStats();

protected:
    virtual void* allocBuffer(size_t minSize) override final;
    virtual int commitBuffer(size_t size) override final;
    virtual const unsigned char* readRaw(void* buf, size_t* inout_len) override final;
    virtual void* getDmaForReading(uint64_t guest_paddr) override final;
    virtual void unlockDma(uint64_t guest_paddr) override final;

    void onSave(android::base::Stream* stream) override;
    unsigned char* onLoad(android::base::Stream* stream) override;

    void type1Read(uint32_t available, char* begin, size_t* count, char** current, const char* ptrEnd);
    void type2Read(uint32_t available, size_t* count, char** current, const char* ptrEnd);
    void type3Read(uint32_t available, size_t* count, char** current, const char* ptrEnd);

    struct asg_context mContext;
    android::emulation::asg::ConsumerCallbacks mCallbacks;

    std::vector<asg_type1_xfer> mType1Xfers;
    std::vector<asg_type2_xfer> mType2Xfers;

    RenderChannel::Buffer mReadBuffer;
    RenderChannel::Buffer mWriteBuffer;
    size_t mReadBufferLeft = 0;

    size_t mXmits = 0;
    size_t mTotalRecv = 0;
    bool mBenchmarkEnabled = false;
    bool mShouldExit = false;
};

}  // namespace emugl
