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

#include "FrameBuffer.h"

#include "android/base/system/System.h"

#include "RenderThreadInfo.h"

#include "GLTestGlobals.h"
#include "GLTestUtils.h"
#include "OpenGLTestContext.h"
#include "OSWindow.h"

using android::base::System;

namespace emugl {

using namespace gltest;

class SampleWindow {
public:
    SampleWindow() {
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
    }

    ~SampleWindow() {
        if (mFb) {
            mFb->finalize();
            delete mFb;
        }
        delete mRenderThreadInfo;
    }

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;
    FrameBuffer* mFb = nullptr;
    RenderThreadInfo* mRenderThreadInfo = nullptr;

    int mWidth = 256;
    int mHeight = 256;
    int mXOffset= 400;
    int mYOffset= 400;
};

} // namespace emugl


int main(int argc, char** argv) {
    emugl::SampleWindow();
    android::base::System::get()->sleepMs(1000);
    return 0;
}
