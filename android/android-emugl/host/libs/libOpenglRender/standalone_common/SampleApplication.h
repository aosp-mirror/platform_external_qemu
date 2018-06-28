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

#pragma once

#include "android/base/Compiler.h"

#include <cinttypes>
#include <functional>
#include <memory>

///////////////////////
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

class FrameBuffer;
class OSWindow;
class RenderThreadInfo;

namespace emugl {

// Determines whether the host GPU should be used.
bool shouldUseHostGpu();

// Creates or adjusts a persistent test window.
// On some systems, test window creation can fail (such as when on a headless server).
// In that case, this function will return nullptr.
OSWindow* createOrGetTestWindow(int xoffset, int yoffset, int width, int height);

// Creates a window (or runs headless) to be used in a sample app.
class SampleApplication {
public:
    SampleApplication(int windowWidth = 256, int windowHeight = 256,
                      int refreshRate = 60);
    ~SampleApplication();

    // A basic draw loop that works similar to most simple
    // GL apps that run on desktop.
    //
    // Per frame:
    //
    // a single GL context for drawing,
    // a color buffer to blit,
    // and a call to post that color buffer.
    void rebind();
    void drawLoop();

    // A more complex loop that uses 3 separate contexts
    // to simulate what goes on in Android:
    //
    // Per frame
    //
    // a GL 'app' context for drawing,
    // a SurfaceFlinger context for rendering the "Layer",
    // and a HWC context for posting.
    void surfaceFlingerComposerLoop();

    // TODO:
    // void HWC2Loop();

protected:
    virtual void initialize() = 0;
    virtual void draw() = 0;

    int mWidth = 256;
    int mHeight = 256;

    int mRefreshRate = 60;

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;

    std::unique_ptr<FrameBuffer> mFb = {};
    std::unique_ptr<RenderThreadInfo> mRenderThreadInfo = {};

    unsigned int mColorBuffer = 0;
    unsigned int mContext = 0;
    unsigned int mSurface = 0;


private:
    // int mWidth = 256;
    // int mHeight = 256;
    // int mRefreshRate = 60;

    // bool mUseSubWindow = false;
    // OSWindow* mWindow = nullptr;
    // std::unique_ptr<FrameBuffer> mFb = {};
    // std::unique_ptr<RenderThreadInfo> mRenderThreadInfo = {};

    int mXOffset= 400;
    int mYOffset= 400;

    // unsigned int mColorBuffer = 0;
    // unsigned int mContext = 0;
    // unsigned int mSurface = 0;

    DISALLOW_COPY_ASSIGN_AND_MOVE(SampleApplication);
};

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


} // namespace emugl
