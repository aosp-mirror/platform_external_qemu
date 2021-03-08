// Copyright (C) 2015 The Android Open Source Project
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

#include "android/opengl/GpuFrameBridge.h"

#include <stdio.h>   // for printf
#include <stdlib.h>  // for NULL, free, malloc
#include <string.h>  // for memcpy

#include <atomic>  // for atomic_bool, memory_o...
#include <memory>  // for unique_ptr

#include "android/base/async/Looper.h"          // for Looper::Timer, Looper
#include "android/base/async/ThreadLooper.h"    // for ThreadLooper
#include "android/base/synchronization/Lock.h"  // for Lock, AutoLock
#include "android/base/synchronization/MessageChannel.h"
#include "android/opengles.h"  // for android_getFlushReadP...

#ifdef _WIN32
#undef ERROR
#endif

namespace android {
namespace opengl {

using android::base::AutoLock;
using android::base::Lock;
using android::base::Looper;
using android::base::MessageChannel;

namespace {

// A small structure to model a single frame of the GPU display,
// as passed between the EmuGL and main loop thread.
struct Frame {
    int width;
    int height;
    void* pixels;
    bool isValid;

    Frame(int w, int h, const void* pixels)
        : width(w), height(h), pixels(NULL), isValid(true) {
        this->pixels = ::malloc(w * 4 * h);
    }

    ~Frame() { ::free(pixels); }
};

// Real implementation of GpuFrameBridge interface.
class Bridge : public GpuFrameBridge {
public:
    // Constructor.
    Bridge()
        : GpuFrameBridge(),
          mRecFrame(NULL),
          mRecTmpFrame(NULL),
          mRecFrameUpdated(false),
          mReadPixelsFunc(android_getReadPixelsFunc()),
          mFlushPixelPipeline(android_getFlushReadPixelPipeline()) {}

    // The gpu bridge receives frames from a buffer that can contain multiple
    // frames. usually the bridge is one frame behind. This is usually not a
    // problem when we have a high enough framerate. However it is possible that
    // the framerate drops really low (even <1). This can result in the bridge
    // never delivering this "stuck frame".
    //
    // As a work around we will flush the reader pipeline if no frames are
    // delivered within at most 2x kFrameDelayms
    const long kMinFPS = 24;
    const long kFrameDelayMs = 1000 / kMinFPS;

    // Destructor
    virtual ~Bridge() {
        if (mRecFrame) {
            delete mRecFrame;
        }
        if (mRecTmpFrame) {
            delete mRecTmpFrame;
        }
    }

    virtual void setLooper(android::base::Looper* aLooper) override {
        mTimer = std::unique_ptr<android::base::Looper::Timer>(
                aLooper->createTimer(_on_frame_notify, this));
    }

    void notify() {
        AutoLock delay(mDelayLock);
        switch (mDelayCallback) {
            case FrameDelay::Reschedule:
                mTimer->startRelative(kFrameDelayMs);
                mDelayCallback = FrameDelay::Scheduled;
                break;
            case FrameDelay::Scheduled:
                mTimer->stop();
                mDelayCallback = FrameDelay::Firing;
                delay.unlock();
                mFlushPixelPipeline(mDisplayId);
                break;
            default:
                assert(false);
        }
    }

    static void _on_frame_notify(void* opaque,
                                 android::base::Looper::Timer* timer) {
        Bridge* worker = static_cast<Bridge*>(opaque);
        worker->notify();
    }

    // Implementation of the GpuFrameBridge::postRecordFrame() method, must be
    // called from the EmuGL thread.
    virtual void postRecordFrame(int width,
                                 int height,
                                 const void* pixels) override {
        postFrame(width, height, pixels, true);
    }

    virtual void postRecordFrameAsync(int width,
                                      int height,
                                      const void* pixels) override {
        postFrame(width, height, pixels, false);
    }

    virtual void* getRecordFrame() override {
        if (mRecFrameUpdated.exchange(false)) {
            AutoLock lock(mRecLock);
            memcpy(mRecFrame->pixels, mRecTmpFrame->pixels,
                   mRecFrame->width * mRecFrame->height * 4);
            mRecFrame->isValid = true;
        }
        return mRecFrame && mRecFrame->isValid ? mRecFrame->pixels : nullptr;
    }

    virtual void* getRecordFrameAsync() override {
        if (mRecFrameUpdated.exchange(false)) {
            AutoLock lock(mRecLock);
            mReadPixelsFunc(mRecFrame->pixels,
                            mRecFrame->width * mRecFrame->height * 4,
                            mDisplayId);
            mRecFrame->isValid = true;
        }
        return mRecFrame && mRecFrame->isValid ? mRecFrame->pixels : nullptr;
    }

    virtual void invalidateRecordingBuffers() override {
        {
            AutoLock lock(mRecLock);
            // Release the buffers because new recording in the furture may have
            // different resolution if multi display changes its resolution.
            if (mRecFrame) {
                delete mRecFrame;
                mRecFrame = nullptr;
            }
            if (mRecTmpFrame) {
                delete mRecTmpFrame;
                mRecTmpFrame = nullptr;
            }
        }
        mRecFrameUpdated.store(false, std::memory_order_release);
    }

    void setFrameReceiver(FrameAvailableCallback receiver,
                          void* opaque) override {
        mReceiver = receiver;
        mReceiverOpaque = opaque;
    }

    void postFrame(int width, int height, const void* pixels, bool copy) {
        {
            AutoLock lock(mRecLock);
            if (!mRecFrame) {
                mRecFrame = new Frame(width, height, pixels);
            }
            if (!mRecTmpFrame) {
                mRecTmpFrame = new Frame(width, height, pixels);
            }
            if (copy) {
                memcpy(mRecTmpFrame->pixels, pixels, width * height * 4);
            }
        }
        mRecFrameUpdated.store(true, std::memory_order_release);
        if (mReceiver) {
            mReceiver(mReceiverOpaque);
            AutoLock delay(mDelayLock);
            switch (mDelayCallback) {
                case FrameDelay::NotScheduled:
                    mTimer->startRelative(kFrameDelayMs);
                    mDelayCallback = FrameDelay::Scheduled;
                    break;
                case FrameDelay::Firing:
                    mDelayCallback = FrameDelay::NotScheduled;
                    break;
                default:
                    mDelayCallback = FrameDelay::Reschedule;
                    break;
            }
        }
    }

    virtual void setDisplayId(uint32_t displayId) override {
        mDisplayId = displayId;
    }
    enum class FrameDelay {
        NotScheduled = 0,  // No delay timer is scheduled
        Scheduled,   // A delay timer has been scheduled and will flush the
                     // pipeline on expiration
        Reschedule,  // Do not flush the pipeline, but reschedule
        Firing,      // A callback has been scheduled, nothing needs to happen
    };

private:
    FrameAvailableCallback mReceiver = nullptr;
    void* mReceiverOpaque = nullptr;
    Lock mRecLock;
    Frame* mRecFrame;
    Frame* mRecTmpFrame;
    std::atomic_bool mRecFrameUpdated;
    ReadPixelsFunc mReadPixelsFunc = 0;
    uint32_t mDisplayId = 0;
    FlushReadPixelPipeline mFlushPixelPipeline = 0;

    std::unique_ptr<android::base::Looper::Timer> mTimer;
    Lock mDelayLock;
    FrameDelay mDelayCallback{FrameDelay::NotScheduled};
};
}  // namespace
// static
GpuFrameBridge* GpuFrameBridge::create() {
    return new Bridge();
}

}  // namespace opengl
}  // namespace android
