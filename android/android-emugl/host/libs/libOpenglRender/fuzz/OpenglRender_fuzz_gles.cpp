// Copyright (C) 2020 The Android Open Source Project
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
#include "FrameBufferTestEnv.h"

#include "OpenglRender/IOStream.h"
#include "OpenglRender/RenderChannel.h"

namespace emugl {

class NullStream final : public IOStream {
public:
    NullStream() : IOStream(4096) { }
    ~NullStream() = default;

    int writeFully(const void* buf, size_t len) override {
        return 0;
    }

    const unsigned char *readFully( void *buf, size_t len) override {
        fprintf(stderr, "%s: FATAL: not intended for use with NullStream\n", __func__);
        abort();
    }

    virtual void* allocBuffer(size_t minSize) override final {
        const size_t kMaxSize = 100 * 1048576;
        if (minSize > kMaxSize) return 0;
        if (mWriteBuffer.size() < minSize) {
            mWriteBuffer.resize_noinit(minSize);
        }
        return mWriteBuffer.data();
    }

    virtual int commitBuffer(size_t size) override final {
        return static_cast<int>(size);
    }

    virtual const unsigned char* readRaw(void* buf, size_t* inout_len) override final {
        fprintf(stderr, "%s: FATAL: not intended for use with NullStream\n", __func__);
        abort();
    }

    virtual void* getDmaForReading(uint64_t guest_paddr) override final {
        return nullptr;
    }

    virtual void unlockDma(uint64_t guest_paddr) override final {
        (void)guest_paddr;
    }

    void onSave(android::base::Stream* stream) override {
    }

    unsigned char* onLoad(android::base::Stream* stream) override {
        return nullptr;
    }

private:
    RenderChannel::Buffer mWriteBuffer;
};

static FrameBufferTestEnv* fbenv = nullptr;
static NullStream* nullStream = nullptr;

static void ensureFbEnv() {
    if (fbenv) return;

    fbenv = setupFrameBufferTestEnv();
    nullStream = new NullStream;
    fbenv->renderThreadInfo->m_gl2Dec.initGL(gles2_dispatch_get_proc_func, nullptr);
}

} // namespace emugl

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    emugl::ensureFbEnv();
    emugl::fbenv->renderThreadInfo->m_gl2Dec.decode((void*)Data, Size, emugl::nullStream, 0);
    return 0;
}
