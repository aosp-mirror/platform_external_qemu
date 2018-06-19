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

#include <gtest/gtest.h>

#include "FrameBuffer.h"

#include "android/base/system/System.h"

#include "GLTestUtils.h"
#include "OpenGLTestContext.h"
#include "OSWindow.h"

using android::base::System;

namespace emugl {

using namespace gltest;

class FrameBufferTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        ASSERT_NE(nullptr, LazyLoadedEGLDispatch::get());
        ASSERT_NE(nullptr, LazyLoadedGLESv2Dispatch::get());

        const int width = 256;
        const int height = 256;
        const int x = 400;
        const int y = 400;

        mWindow = CreateOSWindow();
        mUseSubWindow = mWindow->initialize("FrameBufferTest", width, height);

        bool useHostGpu = System::get()->envGet("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

        if (mUseSubWindow) {
            mWindow->setVisible(true);
            mWindow->setPosition(x, y);
            ASSERT_NE(nullptr, mWindow->getFramebufferNativeWindow());
            mWindow->messageLoop();

            EXPECT_TRUE(
                FrameBuffer::initialize(
                    width, height,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            EXPECT_NE(nullptr, mFb);

            mFb->setupSubWindow(
                (FBNativeWindowType)(uintptr_t)
                mWindow->getFramebufferNativeWindow(),
                0, 0, width, height, width, height, 1.0, 0, false);

            mWindow->messageLoop();
        } else {
            EXPECT_TRUE(
                FrameBuffer::initialize(
                    width, height,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            ASSERT_NE(nullptr, mFb);
        }
    }

    virtual void TearDown() override {
        if (mFb) {
            mFb->finalize();
        }
        if (mWindow) {
            mWindow->destroy();
        }
    }

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;
    FrameBuffer* mFb = nullptr;
};

TEST_F(FrameBufferTest, FrameBufferBasic) {
}

}  // namespace emugl
