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

#include "android/base/async/Looper.h"
#include "android/base/Log.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/opengles.h"

#include <atomic>

#include <stdlib.h>
#include <string.h>

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

    Frame(int w, int h, const void* pixels) :
            width(w), height(h), pixels(NULL), isValid(true) {
        this->pixels = ::malloc(w * 4 * h);
    }

    ~Frame() {
        ::free(pixels);
    }
};

// Real implementation of GpuFrameBridge interface.
class Bridge : public GpuFrameBridge {
public:
    // Constructor.
    Bridge() :
        GpuFrameBridge(),
        mRecFrame(NULL),
        mRecTmpFrame(NULL),
        mRecFrameUpdated(false),
        mReadPixelsFunc(android_getReadPixelsFunc()) { }

    // Destructor
    virtual ~Bridge() {
        if (mRecFrame) {
            delete mRecFrame;
        }
        if (mRecTmpFrame) {
            delete mRecTmpFrame;
        }
    }

    // Implementation of the GpuFrameBridge::postRecordFrame() method, must be
    // called from the EmuGL thread.
    virtual void postRecordFrame(int width, int height, const void* pixels) override {
        postFrame(width, height, pixels, true);
    }

    virtual void postRecordFrameAsync(int width, int height, const void* pixels) override {
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
                            mRecFrame->width * mRecFrame->height * 4);
            mRecFrame->isValid = true;
        }
        return mRecFrame && mRecFrame->isValid ? mRecFrame->pixels : nullptr;
    }

    virtual void invalidateRecordingBuffers() override {
        {
            AutoLock lock(mRecLock);
            if (mRecFrame) {
                mRecFrame->isValid = false;
            }
        }
        mRecFrameUpdated.store(false, std::memory_order_release);
    }

    void setFrameReceiver(FrameAvailableCallback receiver, void* opaque) override {
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
        }
    }

private:
    FrameAvailableCallback mReceiver = nullptr;
    void* mReceiverOpaque = nullptr;
    Lock mRecLock;
    Frame* mRecFrame;
    Frame* mRecTmpFrame;
    std::atomic_bool mRecFrameUpdated;
    ReadPixelsFunc mReadPixelsFunc = 0;
};

}  // namespace

// static
GpuFrameBridge* GpuFrameBridge::create() {
    return new Bridge();
}

}  // namespace opengl
}  // namespace android
