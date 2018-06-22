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

#include "SampleApplication.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/FunctorThread.h"

#include "Standalone.h"

#include "FenceSync.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::MessageChannel;
using android::base::System;
using android::base::TestSystem;

namespace emugl {

// Class holding the persistent test window.
class TestWindow {
public:
    TestWindow() {
        window = CreateOSWindow();
    }

    ~TestWindow() {
        if (window) {
            window->destroy();
        }
    }

    // Check on initialization if windows are available.
    bool initializeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window->initialize("libOpenglRender test", width, height)) {
            window->destroy();
            window = nullptr;
            return false;
        }
        window->setVisible(true);
        window->setPosition(xoffset, yoffset);
        window->messageLoop();
        return true;
    }

    void resizeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window) return;

        window->setPosition(xoffset, yoffset);
        window->resize(width, height);
        window->messageLoop();
    }

    OSWindow* window = nullptr;
};

static LazyInstance<TestWindow> sTestWindow = LAZY_INSTANCE_INIT;

bool shouldUseHostGpu() {
    bool useHost = TestSystem::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    // Also set the global emugl renderer accordingly.
    if (useHost) {
        emugl::setRenderer(SELECTED_RENDERER_HOST);
    } else {
        emugl::setRenderer(SELECTED_RENDERER_SWIFTSHADER_INDIRECT);
    }

    return useHost;
}

bool shouldUseWindow() {
    bool useWindow = TestSystem::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_WINDOW") == "1";
    return useWindow;
}

OSWindow* createOrGetTestWindow(int xoffset, int yoffset, int width, int height) {
    if (!shouldUseWindow()) return nullptr;

    if (!sTestWindow.hasInstance()) {
        sTestWindow.get();
        sTestWindow->initializeWithRect(xoffset, yoffset, width, height);
    } else {
        sTestWindow->resizeWithRect(xoffset, yoffset, width, height);
    }
    return sTestWindow->window;
}

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

// app -> SF queue: separate storage, bindTexture blits
// SF queue -> HWC: shared storage
class ColorBufferQueue { // Note: we could have called this BufferQueue but there is another
                         // class of name BufferQueue that does something totally different

public:
    static constexpr int kCapacity = 3;
    struct Item {
        unsigned int colorBuffer;
        FenceSync* sync;
    };

    ColorBufferQueue() = default;

    void queueBuffer(const Item& item) {
        mQueue.send(item);
    }

    void dequeueBuffer(Item* outItem) {
        mQueue.receive(outItem);
    }

private:
    MessageChannel<Item, kCapacity> mQueue;
};

class ScopedRender {
public:
    ScopedRender(unsigned int context, unsigned int surface) :
        mThreadInfo(new RenderThreadInfo()) {
        FrameBuffer* fb = FrameBuffer::getFB();
        fb->bindContext(context, surface, surface);
    }

    ~ScopedRender() {
        FrameBuffer::getFB()->bindContext(0, 0, 0);
        delete mThreadInfo;
    }
private:
    RenderThreadInfo* mThreadInfo;
};

// SampleApplication implementation/////////////////////////////////////////////
SampleApplication::SampleApplication(int windowWidth, int windowHeight, int refreshRate) :
    mWidth(windowWidth), mHeight(windowHeight), mRefreshRate(refreshRate) {

    setupStandaloneLibrarySearchPaths();

    LazyLoadedEGLDispatch::get();
    LazyLoadedGLESv2Dispatch::get();

    bool useHostGpu = shouldUseHostGpu();
    mWindow = createOrGetTestWindow(mXOffset, mYOffset, mWidth, mHeight);
    mUseSubWindow = mWindow != nullptr;

    FrameBuffer::initialize(
            mWidth, mHeight,
            mUseSubWindow,
            !useHostGpu /* egl2egl */);
    if (mUseSubWindow) {
        mFb = FrameBuffer::getFB();

        mFb->setupSubWindow(
            (FBNativeWindowType)(uintptr_t)
            mWindow->getFramebufferNativeWindow(),
            0, 0,
            mWidth, mHeight, mWidth, mHeight,
            mWindow->getDevicePixelRatio(), 0, false);
        mWindow->messageLoop();
    } else {
            FrameBuffer::initialize(
                mWidth, mHeight,
                mUseSubWindow,
                !useHostGpu /* egl2egl */);
        mFb = FrameBuffer::getFB();
    }

    mRenderThreadInfo = new RenderThreadInfo();

    mColorBuffer = mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    mContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
    mSurface = mFb->createWindowSurface(0, mWidth, mHeight);

    mFb->bindContext(mContext, mSurface, mSurface);
    mFb->setWindowSurfaceColorBuffer(mSurface, mColorBuffer);
}

SampleApplication::~SampleApplication() {
    if (mFb) {
        mFb->bindContext(0, 0, 0);
        mFb->closeColorBuffer(mColorBuffer);
        mFb->DestroyWindowSurface(mSurface);
        mFb->finalize();
        delete mFb;
    }
    delete mRenderThreadInfo;
}

void SampleApplication::drawLoop() {
    this->initialize();

    Vsync vsync(mRefreshRate);

    while (true) {
        this->draw();
        mFb->flushWindowSurfaceColorBuffer(mSurface);
        vsync.waitUntilNextVsync();
        if (mUseSubWindow) {
            mFb->post(mColorBuffer);
            mWindow->messageLoop();
        }
    }
}

void SampleApplication::surfaceFlingerComposerLoop() {
    fprintf(stderr, "%s: call\n", __func__);

    // create enough color buffers, window surfaces, and render contexts
    // for everyone

    ColorBufferQueue app2sfQueue;
    ColorBufferQueue sf2appQueue;
    ColorBufferQueue sf2hwcQueue;
    ColorBufferQueue hwc2sfQueue;

    std::vector<unsigned int> sfColorBuffers;
    std::vector<unsigned int> hwcColorBuffers;

    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        sfColorBuffers.push_back(mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE));
        hwcColorBuffers.push_back(mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE));
    }

    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        mFb->openColorBuffer(sfColorBuffers[i]);
        mFb->openColorBuffer(hwcColorBuffers[i]);
    }

    unsigned int appSurface = mFb->createWindowSurface(0, mWidth, mHeight);
    unsigned int sfSurface = mFb->createWindowSurface(0, mWidth, mHeight);
    unsigned int hwcSurface = mFb->createWindowSurface(0, mWidth, mHeight);

    unsigned int appContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
    unsigned int sfContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
    unsigned int hwcContext = mFb->createRenderContext(0, 0, GLESApi_3_0);

    // prime the queue
    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        sf2appQueue.push_back({ sfColorBuffers[i], nullptr });
        hwc2sfQueue.push_back({ sfColorBuffers[i], nullptr });
    }

    FunctorThread appThread([&]() {
        ScopedRender(appContext, appSurface);
        ColorBufferQueue::Item item;
        sf2appQueue.dequeueBuffer(&item);

        mFb->setWindowSurfaceColorBuffer(surface, item.colorBuffer);
        this->initialize();

        while (true) {
            this->draw();
            mFb->flushWindowSurfaceColorBuffer(appSurface);
            FenceSync* sync = new FenceSync(false /* no native fence */, true /* destroy when signaled */);
            app2sfQueue.queueBuffer({ item.colorBuffer, sync }); 
            sf2appQueue.dequeueBuffer(item);
            mFb->setWindowSurfaceColorBuffer(surface, item.colorBuffer);
        }
    });

    FunctorThread sfThread([&]() {
        ScopedRender(sfContext, sfSurface);
        auto gl = LazyLoadedGLESv2Dispatch::get();
        // TODO: initialize

        GLint blitTexture;
        gl->glGenTextures(1, &blitTexture);
        gl->glBindTexture(GL_TEXTURE_2D, blitTexture);

        ColorBufferQueue::Item appItem;
        ColorBufferQueue::Item hwcItem;

        while (true) {
            hwc2sfQueue.dequeueBuffer(&hwcItem);
            mFb->setWindowSurfaceColorBuffer(sfSurface, hwcItem.colorBuffer);

            app2sfQueue.dequeueBuffer(&appItem);
            mFb->bindColorBufferToTexture(appItem.colorBuffer);

            // TODO: render it as a quad

            mFb->flushWindowSurfaceColorBuffer(sfSurface);
            sf2hwcQueue.queueBuffer({ hwcItem.colorBuffer, nullptr});
        }
    });

    FunctorThread hwcThread([&]() {
        Vsync vsync(mRefreshRate);
        // ScopedRender(hwcContext, hwcSurface);

        ColorBufferQueue::Item sfItem;
        while (true) {
            sf2hwcQueue.dequeueBuffer(&sfItem);
            vsync.waitUntilNextVsync();
            if (mUseSubWindow) {
                mFb->post(sfItem.colorBuffer);
                hwc2sfQueue.queueBuffer({ sfItem.colorBuffer, nullptr });
                mWindow->messageLoop();
            }
        }
    });

    hwcThread.start();
    sfThread.start();
    appThread.start();

    appThread.wait();
    sfThread.wait();
    hwcThread.wait();
}

} // namespace emugl
