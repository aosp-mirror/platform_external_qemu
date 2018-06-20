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

#include "RenderThreadInfo.h"

#include "GLTestGlobals.h"
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

        bool useHostGpu = shouldUseHostGpu();
        mWindow = createOrGetTestWindow(mXOffset, mYOffset, mWidth, mHeight);
        mUseSubWindow = mWindow != nullptr;

        if (mUseSubWindow) {
            ASSERT_NE(nullptr, mWindow->getFramebufferNativeWindow());

            EXPECT_TRUE(
                FrameBuffer::initialize(
                    mWidth, mHeight,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            EXPECT_NE(nullptr, mFb);

            mFb->setupSubWindow(
                (FBNativeWindowType)(uintptr_t)
                mWindow->getFramebufferNativeWindow(),
                0, 0,
                mWidth, mHeight, mWidth, mHeight,
                mWindow->getDevicePixelRatio(), 0, false);
            mWindow->messageLoop();
        } else {
            EXPECT_TRUE(
                FrameBuffer::initialize(
                    mWidth, mHeight,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            ASSERT_NE(nullptr, mFb);
        }

        mRenderThreadInfo = new RenderThreadInfo();
    }

    virtual void TearDown() override {
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

TEST_F(FrameBufferTest, FrameBufferBasic) {
}

TEST_F(FrameBufferTest, CreateColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    // FramBuffer::finalize handles color buffer destruction here
}

TEST_F(FrameBufferTest, CreateCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    mFb->closeColorBuffer(handle);
}

TEST_F(FrameBufferTest, CreateOpenCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));
    mFb->closeColorBuffer(handle);
}

TEST_F(FrameBufferTest, CreateOpenUpdateCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));

    TestTexture forUpdate = createTestPatternRGBA8888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forUpdate.data());

    TestTexture forRead = createTestTextureRGBA8888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

// bug: 110105029
TEST_F(FrameBufferTest, CreateOpenUpdateCloseColorBuffer_FormatChange) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));

    TestTexture forUpdate = createTestPatternRGB888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, forUpdate.data());

    TestTexture forRead = createTestTextureRGB888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

TEST_F(FrameBufferTest, Configs) {
    const FbConfigList* configs = mFb->getConfigs();
    EXPECT_GE(configs->size(), 0);
}

TEST_F(FrameBufferTest, CreateRenderContext) {
    HandleType handle = mFb->createRenderContext(0, 0, GLESApi_3_0);
    EXPECT_NE(0, handle);
}

TEST_F(FrameBufferTest, CreateWindowSurface) {
    HandleType handle = mFb->createWindowSurface(0, mWidth, mHeight);
    EXPECT_NE(0, handle);
}

TEST_F(FrameBufferTest, CreateBindRenderContext) {
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);
    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
}

TEST_F(FrameBufferTest, BasicBlit) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    HandleType colorBuffer =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);

    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
    EXPECT_TRUE(mFb->setWindowSurfaceColorBuffer(surface, colorBuffer));

    float colors[3][4] = {
        { 1.0f, 0.0f, 0.0f, 1.0f},
        { 0.0f, 1.0f, 0.0f, 1.0f},
        { 0.0f, 0.0f, 1.0f, 1.0f},
    };

    for (int i = 0; i < 3; i++) {
        float* color = colors[i];

        gl->glClearColor(color[0], color[1], color[2], color[3]);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mFb->flushWindowSurfaceColorBuffer(surface);

        TestTexture targetBuffer =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, color[0], color[1], color[2], color[3]);

        TestTexture forRead =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);

        mFb->readColorBuffer(
            colorBuffer, 0, 0, mWidth, mHeight,
            GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

        EXPECT_TRUE(
            ImageMatches(
                mWidth, mHeight, 4, mWidth,
                targetBuffer.data(), forRead.data()));

        if (mUseSubWindow) {
            mFb->post(colorBuffer);
            mWindow->messageLoop();
        }
    }

    EXPECT_TRUE(mFb->bindContext(0, 0, 0));
    mFb->closeColorBuffer(colorBuffer);
    mFb->closeColorBuffer(colorBuffer);
    mFb->DestroyWindowSurface(surface);
}

}  // namespace emugl
