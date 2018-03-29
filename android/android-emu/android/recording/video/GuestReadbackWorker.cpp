// Copyright (C) 2018 The Android Open Source Project
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

#include "android/recording/video/GuestReadbackWorker.h"

#include "android/base/synchronization/Lock.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/control/display_agent.h"
#include "android/framebuffer.h"
#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__);

namespace android {
namespace recording {

using android::RecursiveScopedVmLock;
using android::base::AutoLock;
using android::base::Lock;

namespace {
// Helper class to readback guest video frames. The posted frames are
// triple-buffered to allow a consumer to read the latest frame while new
// results are using a double-buffered solution.
class GuestReadbackWorkerImpl : public GuestReadbackWorker {
public:
    explicit GuestReadbackWorkerImpl(const QAndroidDisplayAgent* agent,
                                     uint32_t fbWidth,
                                     uint32_t fbHeight)
        : mAgent(agent), mFbWidth(fbWidth), mFbHeight(fbHeight) {
        // determine which format the framebuffer is using
        int bpp = 0;
        mAgent->getFrameBuffer(nullptr, nullptr, nullptr, &bpp, nullptr);
        assert(bpp == 2 || bpp == 4);
        mFormat = bpp == 2 ? VideoFormat::RGB565 : VideoFormat::BGRA8888;
        auto sz = fbWidth * fbHeight * bpp;

        // Preallocate the space for the three frames
        for (int i = 0; i < 3; ++i) {
            mFrames[i] = Frame(sz);
            mFrames[i].format.videoFormat = mFormat;
        }

        mAgent->registerUpdateListener(&onPost, this);

        // force a repost so we can get the first frame
        qframebuffer_invalidate_all();
        // qframebuffer_check_updates() must be under the
        // qemu_mutex_iothread_lock so graphic_hw_update() can update the
        // memory section for the framebuffer.
        {
            RecursiveScopedVmLock lock;
            qframebuffer_check_updates();
        }
    }

    // Called by the consumer.
    virtual const Frame* getFrame() override {
        AutoLock lock(mLock);
        if (!mFirstFrameLoaded) {
            return nullptr;
        }

        // Update the index to the last written frame.
        mCopyIdx = mNewestIdx;
        lock.unlock();

        mFrames[mCopyIdx].tsUs =
                android::base::System::get()->getHighResTimeUs();
        return &mFrames[mCopyIdx];
    }

    virtual VideoFormat getPixelFormat() override { return mFormat; }

    virtual ~GuestReadbackWorkerImpl() {
        if (mAgent) {
            mAgent->unregisterUpdateListener(&onPost);
        }
    }

private:
    static void onPost(void* opaque, int x, int y, int width, int height) {
        D("%s(x=%d, y=%d, w=%d, h=%d)", __func__, x, y, width, height);
        auto g = reinterpret_cast<GuestReadbackWorkerImpl*>(opaque);

        if (width <= 0 || height <= 0) {
            return;
        }

        int w = 0, h = 0, bpp = 0;
        unsigned char* px = nullptr;
        g->mAgent->getFrameBuffer(&w, &h, nullptr, &bpp, &px);
        if (!px || w <= 0 || h <= 0) {
            return;
        }

        AutoLock lock(g->mLock);
        if (!g->mFirstFrameLoaded) {
            g->mFrames[0].dataVec.assign(px, px + g->mFrames[0].dataVec.size());
            g->mFirstFrameLoaded = true;
            return;
        }

        // Need to figure out which frame we can write into. If mCopyIdx and
        // mNewestIdx are the same, then we have two buffers to work with. If
        // they are different, then we only have one choice.
        uint8_t writeIdx;
        if (g->mCopyIdx == g->mNewestIdx) {
            writeIdx = (g->mCopyIdx + 1) % 3;
        } else {
            writeIdx = 3 - (g->mCopyIdx + g->mNewestIdx);
        }
        lock.unlock();

        g->mFrames[writeIdx].dataVec.assign(
                px, px + g->mFrames[writeIdx].dataVec.size());

        lock.lock();
        g->mNewestIdx = writeIdx;
    }

private:
    const QAndroidDisplayAgent* mAgent = nullptr;
    uint32_t mFbWidth = 0;
    uint32_t mFbHeight = 0;
    VideoFormat mFormat = VideoFormat::INVALID_FMT;
    Lock mLock;
    Frame mFrames[3];
    bool mFirstFrameLoaded = false;
    bool mIsCopying = false;
    uint8_t mCopyIdx = 0;
    uint8_t mNewestIdx = 0;
};
}  // namespace

std::unique_ptr<GuestReadbackWorker> GuestReadbackWorker::create(
        const QAndroidDisplayAgent* agent,
        uint32_t fbWidth,
        uint32_t fbHeight) {
    return std::unique_ptr<GuestReadbackWorker>(
            new GuestReadbackWorkerImpl(agent, fbWidth, fbHeight));
}

}  // namespace recording
}  // namespace android
