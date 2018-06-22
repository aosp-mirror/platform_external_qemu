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

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::System;
using android::base::TestSystem;
using android::base::FunctorThread;

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

} // namespace emugl
