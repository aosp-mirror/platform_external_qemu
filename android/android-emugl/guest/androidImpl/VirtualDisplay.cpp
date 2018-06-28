// Copyright 2018 The Android Open Source Project
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
// limitations under the License.#pragma once
#include "VirtualDisplay.h"

#include "GoldfishOpenGLDispatch.h"
#include "GrallocDispatch.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include <unordered_map>

#include <system/window.h>
#include <stdio.h>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::MessageChannel;
using android::base::System;

namespace aemu {

class BQ {
public:
    static constexpr int kCapacity = 3;

    class Item {
    public:
        Item(ANativeWindowBuffer* buf = 0, int fd = -1) :
            buffer(buf), fenceFd(fd) { }
        ANativeWindowBuffer* buffer = 0;
        int fenceFd = -1;
    };

    void queueBuffer(const Item& item) {
        mQueue.send(item);
    }

    void dequeueBuffer(Item* outItem) {
        mQueue.receive(outItem);
    }

private:
    MessageChannel<Item, kCapacity> mQueue;
};



static int hook_setSwapInterval(struct ANativeWindow* window, int interval);
static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer);
static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_query(const struct ANativeWindow* window, int what, int* value);
static int hook_perform(struct ANativeWindow* window, int operation, ... );
static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd);
static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static void hook_incRef(struct android_native_base_t* common);
static void hook_decRef(struct android_native_base_t* common);

class VirtualSurface : public ANativeWindow {
public:
    VirtualSurface() {
        // Initialize the ANativeWindow function pointers.
        ANativeWindow::setSwapInterval  = hook_setSwapInterval;
        ANativeWindow::dequeueBuffer    = hook_dequeueBuffer;
        ANativeWindow::cancelBuffer     = hook_cancelBuffer;
        ANativeWindow::queueBuffer      = hook_queueBuffer;
        ANativeWindow::query            = hook_query;
        ANativeWindow::perform          = hook_perform;

        ANativeWindow::dequeueBuffer_DEPRECATED = hook_dequeueBuffer_DEPRECATED;
        ANativeWindow::cancelBuffer_DEPRECATED  = hook_cancelBuffer_DEPRECATED;
        ANativeWindow::lockBuffer_DEPRECATED    = hook_lockBuffer_DEPRECATED;
        ANativeWindow::queueBuffer_DEPRECATED   = hook_queueBuffer_DEPRECATED;

        const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
        const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

        common.incRef = hook_incRef;
        common.decRef = hook_decRef;
    }

    void setProducer(BQ* fromSf, BQ* toSf) {
        mFromSf = fromSf;
        mToSf = toSf;
    }

    int dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd) {
        assert(mFromSf);
        BQ::Item item;
        mFromSf->dequeueBuffer(&item);
        *buffer = item.buffer;
        *fenceFd = item.fenceFd;
        return 0;
    }

    int queueBuffer(ANativeWindowBuffer* buffer, int fenceFd) {
        assert(mToSf);
        mToSf->queueBuffer({ buffer, fenceFd });
        return 0;
    }

    BQ* mFromSf = nullptr;
    BQ* mToSf = nullptr;

    int swapInterval = 1;

    static VirtualSurface* getSelf(ANativeWindow* window) { return (VirtualSurface*)(window); }
};

// Android native window implementation
static int hook_setSwapInterval(struct ANativeWindow* window, int interval) {
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    surface->swapInterval = interval;
    return 0;
}

static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_query(const struct ANativeWindow* window, int what, int* value) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_perform(struct ANativeWindow* window, int operation, ... ) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    return surface->dequeueBuffer(buffer, fenceFd);
}

static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    return surface->queueBuffer(buffer, fenceFd);
}

static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static void hook_incRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}

static void hook_decRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}

class VirtualDisplay {
public:
    class Vsync {
    public:
        Vsync(int refreshRate = 60) :
            mRefreshRate(refreshRate),
            mRefreshIntervalUs(1000000ULL / mRefreshRate),
            mThread([this] {
                while (true) {
                    if (mShouldStop) return 0;
                    System::get()->sleepUs(mRefreshIntervalUs);
                    AutoLock lock(mLock);
                    mSync = 1;
                    mCv.signal();
                }
                return 0;
            }) {
            mThread.start();
        }
    
        ~Vsync() {
            mShouldStop = true;
        }
    
        void waitUntilNextVsync() {
            AutoLock lock(mLock);
            mSync = 0;
            while (!mSync) {
                mCv.wait(&mLock);
            }
        }
    
    private:
        int mShouldStop = false;
        int mRefreshRate = 60;
        uint64_t mRefreshIntervalUs;
        volatile int mSync = 0;
    
        Lock mLock;

        ConditionVariable mCv;
    
        FunctorThread mThread;
    };

    VirtualDisplay(int width = 256, int height = 256) : mWidth(width), mHeight(height) {
        mGralloc = *emugl::loadGoldfishGralloc();
        mFb0Device = mGralloc.open_device("fb0");
        mGpu0Device = mGralloc.open_device("gpu0");

        primeQueue(&hwc2sf, GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_FB);
    }

    void primeQueue(BQ* queue, int usage) {
        bool useFbDevice = usage & GRALLOC_USAGE_HW_FB;
        for (int i = 0; i < BQ::kCapacity; i++) {
            queue->queueBuffer(
                { allocateWindowBuffer(
                        useFbDevice,
                      mWidth, mHeight,
                      HAL_PIXEL_FORMAT_RGBA_8888 /* rgba_8888 */,
                      usage), -1 });
        }
    }

    ANativeWindowBuffer* allocateWindowBuffer(bool forFb, int width, int height, int format, int usage) {
        buffer_handle_t* handle = new buffer_handle_t;
        int stride;
        mGralloc.alloc(forFb ? mFb0Device : mGpu0Device, width, height, format, usage, handle, &stride);

        ANativeWindowBuffer* buffer = new ANativeWindowBuffer;

        buffer->width = width;
        buffer->height = height;
        buffer->stride = stride;
        buffer->format = format;
        buffer->usage_deprecated = usage;
        buffer->usage = usage;

        buffer->handle = *handle;

        return buffer;
    }

    int createAppWindowAndSetCurrent() {
        int nextUnusedWindowId = 0;

        for (auto it : mWindows) {
            if (it.first >= nextUnusedWindowId) {
                nextUnusedWindowId = it.first + 1;
            }
        }

        Window w = { new VirtualSurface, new BQ, new BQ };
        mWindows[nextUnusedWindowId] = w;

        auto window = mWindows[nextUnusedWindowId];

        window.surface->setProducer(window.sf2appQueue, window.app2sfQueue);

        primeQueue(window.sf2appQueue, GRALLOC_USAGE_HW_RENDER);

        mCurrentWindow = nextUnusedWindowId;
        return nextUnusedWindowId;
    }

    ANativeWindow* getCurrentWindow() {
        return (ANativeWindow*)(mWindows[mCurrentWindow].surface);
    }

private:
    int mWidth;
    int mHeight;

    GrallocDispatch mGralloc;
    void* mFb0Device;
    void* mGpu0Device;

    struct Window {
        VirtualSurface* surface;
        BQ* sf2appQueue;
        BQ* app2sfQueue;
    };

    std::unordered_map<int, Window> mWindows;
    int mCurrentWindow = -1;

    BQ sf2hwc;
    BQ hwc2sf;
};

static LazyInstance<VirtualDisplay> sDisplay = LAZY_INSTANCE_INIT;

void initVirtualDisplay() {
    sDisplay.get();
}

} // namespace aemu
