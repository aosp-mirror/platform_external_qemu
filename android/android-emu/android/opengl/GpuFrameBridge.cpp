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
        this->pixels = ::calloc(w * 4, h);
        ::memcpy(this->pixels, pixels, w * 4 * h);
    }

    ~Frame() {
        ::free(pixels);
    }
};

// Real implementation of GpuFrameBridge interface.
class Bridge : public GpuFrameBridge {
public:
    // Constructor.
    Bridge(Looper* looper, Callback* callback, void* callbackOpaque) :
            GpuFrameBridge(),
            mLooper(looper),
            mInSocket(-1),
            mOutSocket(-1),
            mFdWatch(NULL),
            mFrames(),
            mCallback(callback),
            mCallbackOpaque(callbackOpaque),
            mRecFrame(NULL),
            mRecTmpFrame(NULL),
            mRecFrameUpdated(false),
            mReadPixelsFunc(android_getReadPixelsFunc()) {
        if (!mLooper) {
            return;
        }

        if (::android::base::socketCreatePair(&mInSocket, &mOutSocket) < 0) {
            PLOG(ERROR) << "Could not create socket pair";
            return;
        }

        mFdWatch = looper->createFdWatch(mOutSocket, onSocketEvent, this);
        if (!mFdWatch) {
            LOG(ERROR) << "Could not create FdWatch";
            android::base::socketClose(mInSocket);
            android::base::socketClose(mOutSocket);
            mInSocket = -1;
            mOutSocket = -1;
            return;
        }

        mFdWatch->wantRead();
    }

    // Destructor
    virtual ~Bridge() {
        // Non-recording mode
        if (mLooper) {
            delete mFdWatch;
            android::base::socketClose(mOutSocket);
            android::base::socketClose(mInSocket);
        }
        if (mRecFrame) {
            delete mRecFrame;
        }
        if (mRecTmpFrame) {
            delete mRecTmpFrame;
        }
    }

    // Implementation of the GpuFrameBridge::postFrame() method, must be
    // called from the EmuGL thread.
    virtual void postFrame(int width, int height, const void* pixels) {
        if (mInSocket < 0) {
            return;
        }
        Frame* frame = new Frame(width, height, pixels);
        ::memcpy(frame->pixels, pixels, width * 4 * height);
        mFrames.send(frame);
        char c = 1;
        android::base::socketSend(mInSocket, &c, 1);
    }

    // Implementation of the GpuFrameBridge::postRecordFrame() method, must be
    // called from the EmuGL thread.
    virtual void postRecordFrame(int width, int height, const void* pixels) {
        postRecordFrameAsync(width, height, pixels);
        AutoLock lock(mRecLock);
        memcpy(mRecTmpFrame->pixels, pixels, width * height * 4);
    }

    virtual void postRecordFrameAsync(int width, int height, const void* pixels) {
        {
            AutoLock lock(mRecLock);
            if (!mRecFrame) {
                mRecFrame = new Frame(width, height, pixels);
            }
            if (!mRecTmpFrame) {
                mRecTmpFrame = new Frame(width, height, pixels);
            }
        }
        mRecFrameUpdated.store(true, std::memory_order_release);
    }

    virtual void* getRecordFrame() {
        if (mRecFrameUpdated.exchange(false)) {
            AutoLock lock(mRecLock);
            memcpy(mRecFrame->pixels, mRecTmpFrame->pixels,
                   mRecFrame->width * mRecFrame->height * 4);
            mRecFrame->isValid = true;
        }
        return mRecFrame && mRecFrame->isValid ? mRecFrame->pixels : nullptr;
    }

    virtual void* getRecordFrameAsync() {
        if (mRecFrameUpdated.exchange(false)) {
            AutoLock lock(mRecLock);
            mReadPixelsFunc(mRecFrame->pixels,
                            mRecFrame->width * mRecFrame->height * 4);
            mRecFrame->isValid = true;
        }
        return mRecFrame && mRecFrame->isValid ? mRecFrame->pixels : nullptr;
    }

    virtual void invalidateRecordingBuffers() {
        {
            AutoLock lock(mRecLock);
            if (mRecFrame) {
                mRecFrame->isValid = false;
            }
        }
        mRecFrameUpdated.store(false, std::memory_order_release);
    }

private:
    enum {
        kMaxFrames = 16
    };

    // Called from the looper thread when a new Frame instance is available.
    static void onSocketEvent(void* opaque, int fd, unsigned events) {
        Bridge* bridge = reinterpret_cast<Bridge*>(opaque);
        if (events & Looper::FdWatch::kEventRead) {
            char c = 0;
            android::base::socketRecv(bridge->mOutSocket, &c, 1);
            // char c; is the "confirmation" bit
            // that actual data was grabbed by the socket.
            // In a more multithreaded situation,
            // we aren't guaranteed 1:1
            // onSocketEvent and postFrame calls.
            // If we simply quit if the confirm bit is not set,
            // we can avoid a deadlock.
            if (!c) {
                return;
            }
            Frame* frame = NULL;
            bridge->mFrames.receive(&frame);
            if (frame) {
                bridge->mCallback(bridge->mCallbackOpaque,
                                  frame->width,
                                  frame->height,
                                  frame->pixels);
                bridge->postRecordFrame(frame->width,
                                        frame->height,
                                        frame->pixels);
                delete frame;
            }
        }
    }

    Looper* mLooper;
    int mInSocket;
    int mOutSocket;
    Looper::FdWatch* mFdWatch;
    MessageChannel<Frame*, kMaxFrames> mFrames;
    Callback* mCallback;
    void* mCallbackOpaque;
    Lock mRecLock;
    Frame* mRecFrame;
    Frame* mRecTmpFrame;
    std::atomic_bool mRecFrameUpdated;

    ReadPixelsFunc mReadPixelsFunc = 0;
};

}  // namespace

// static
GpuFrameBridge* GpuFrameBridge::create(android::base::Looper* looper,
                                       Callback* callback,
                                       void* callbackOpaque) {
    return new Bridge(looper, callback, callbackOpaque);
}

}  // namespace opengl
}  // namespace android
