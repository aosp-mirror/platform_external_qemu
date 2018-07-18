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

#include "Standalone.h"
#include "GLTestUtils.h"

#include <memory>

using android::base::System;

namespace emugl {

class ClearColor final : public SampleApplication {
public:
    ClearColor() : SampleApplication(256, 256, 60) { }

    ~ClearColor() {
        auto gl = LazyLoadedGLESv2Dispatch::get();
        if (mFbo) {
            gl->glDeleteFramebuffers(1, &mFbo);
            gl->glDeleteTextures(1, &mTexture);
        }
    }

    void setClearColor(float r, float g, float b, float a) {
        mRed = r;
        mGreen = g;
        mBlue = b;
        mAlpha = a;
    }

    void setUseFbo(bool useFboDraw, bool useFboRead) {
        mUseFboDraw = useFboDraw;
        mUseFboRead = useFboRead;
    }

    void initialize() override {
        auto gl = LazyLoadedGLESv2Dispatch::get();
        gl->glActiveTexture(GL_TEXTURE0);

        if (!mFbo) {
            gl->glGenFramebuffers(1, &mFbo);
            gl->glGenTextures(1, &mTexture);

            gl->glBindTexture(GL_TEXTURE_2D, mTexture);
            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             mWidth, mHeight, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, 0);
            gl->glBindTexture(GL_TEXTURE_2D, 0);

            gl->glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, mTexture, 0);
            gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void draw() override {
        auto gl = LazyLoadedGLESv2Dispatch::get();

        gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (mUseFboDraw) {
            gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFbo);
        }
        if (mUseFboRead) {
            gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, mFbo);
        }

        gl->glClearColor(mRed, mGreen, mBlue, mAlpha);
        gl->glClear(GL_COLOR_BUFFER_BIT);
    }

    void setNonDefaultDrawFbo() {
        setUseFbo(true, mUseFboRead);
    }

    void setNonDefaultReadFbo() {
        setUseFbo(mUseFboDraw, true);
    }

    void drawWithColor(const float drawColor[4]) {
        setClearColor(drawColor[0], drawColor[1], drawColor[2], drawColor[3]);
        drawOnce();
    }

    void verifySwappedColor(const float wantedColor[4]) {
        TestTexture targetBuffer =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, wantedColor[0], wantedColor[1], wantedColor[2], wantedColor[3]);

        TestTexture forRead =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);

        mFb->readColorBuffer(
            mColorBuffer, 0, 0, mWidth, mHeight,
            GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

        EXPECT_TRUE(
            ImageMatches(mWidth, mHeight, 4, mWidth, targetBuffer.data(), forRead.data()));
    }

private:
    bool mUseFboDraw = false;
    bool mUseFboRead = false;

    GLuint mFbo = 0;
    GLuint mTexture = 0;

    float mRed = 1.0f;
    float mGreen = 1.0f;
    float mBlue = 1.0f;
    float mAlpha = 1.0f;
};

class DefaultFramebufferBlitTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        mApp.reset(new ClearColor());
    }

    virtual void TearDown() override {
        mApp.reset();
        EXPECT_EQ(EGL_SUCCESS, LazyLoadedEGLDispatch::get()->eglGetError())
                << "DefaultFramebufferBlitTest TearDown found an EGL error";
    }

    std::unique_ptr<ClearColor> mApp;
};

static constexpr float kDrawColorRed[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr float kDrawColorGreen[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

TEST_F(DefaultFramebufferBlitTest, DefaultDrawDefaultRead) {
    // Draw to default framebuffer; color should show up
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);
}

TEST_F(DefaultFramebufferBlitTest, NonDefaultDrawNonDefaultRead) {
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);

    // Color should not be affected by the draw to non-default
    // draw/read framebuffers.
    // (we preserve the previous contents of the surface in
    // SampleApplication::drawOnce)
    mApp->setNonDefaultDrawFbo();
    mApp->setNonDefaultReadFbo();

    mApp->drawWithColor(kDrawColorGreen);
    mApp->verifySwappedColor(kDrawColorRed);
}

TEST_F(DefaultFramebufferBlitTest, DefaultDrawNonDefaultRead) {
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);

    // Bug: 110996998
    // Draw to default framebuffer, and have a non-default
    // read framebuffer bound. Color should show up
    mApp->setNonDefaultReadFbo();

    mApp->drawWithColor(kDrawColorGreen);
    mApp->verifySwappedColor(kDrawColorGreen);
}


TEST_F(DefaultFramebufferBlitTest, NonDefaultDrawDefaultRead) {
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);

    // Draw to non-default framebuffer, and have the default
    // read framebuffer bound. Color should not show up.
    // (we preserve the previous contents of the surface in
    // SampleApplication::drawOnce)
    mApp->setNonDefaultDrawFbo();

    mApp->drawWithColor(kDrawColorGreen);
    mApp->verifySwappedColor(kDrawColorRed);
}

}  // namespace emugl
