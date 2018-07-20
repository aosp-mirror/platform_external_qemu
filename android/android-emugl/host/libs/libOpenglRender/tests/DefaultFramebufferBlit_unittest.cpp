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

struct ClearColorParam {
    const GLESApi glVersion;
    const bool fastBlit;

    ClearColorParam(GLESApi glVersion, bool fastBlit)
        : glVersion(glVersion), fastBlit(fastBlit) {}
};

static void PrintTo(const ClearColorParam& param, std::ostream* os) {
    *os << "ClearColorParam(";

    switch (param.glVersion) {
        case GLESApi_CM:  *os << "GLESApi_CM";  break;
        case GLESApi_2:   *os << "GLESApi_2";   break;
        case GLESApi_3_0: *os << "GLESApi_3_0"; break;
        case GLESApi_3_1: *os << "GLESApi_3_1"; break;
        case GLESApi_3_2: *os << "GLESApi_3_2"; break;
        default: *os << "GLESApi(" << int(param.glVersion) << ")"; break;
    }

    *os << ", " << (param.fastBlit ? "fast blit" : "slow blit") << ")";
}

class ClearColor final : public SampleApplication {
public:
    ClearColor(ClearColorParam param) : SampleApplication(256, 256, 60, param.glVersion) {
        if (!param.fastBlit) {
            // Disable fast blit and then recreate the color buffer to apply the
            // change.
            mFb->disableFastBlit();

            mFb->closeColorBuffer(mColorBuffer);
            mColorBuffer = mFb->createColorBuffer(
                    mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
            mFb->setWindowSurfaceColorBuffer(mSurface, mColorBuffer);
        }
    }

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

    void setUseFboCombined(bool useFbo) {
        mUseFboCombined = useFbo;
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

        gl->glBindFramebuffer(GL_FRAMEBUFFER, mUseFboCombined ? mFbo : 0);

        if (mUseFboDraw) {
            gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFbo);
        }
        if (mUseFboRead) {
            gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, mFbo);
        }

        gl->glClearColor(mRed, mGreen, mBlue, mAlpha);
        gl->glClear(GL_COLOR_BUFFER_BIT);
    }

    void setNonDefaultCombinedFbo() {
        setUseFboCombined(true);
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
    bool mUseFboCombined = false;
    bool mUseFboDraw = false;
    bool mUseFboRead = false;

    GLuint mFbo = 0;
    GLuint mTexture = 0;

    float mRed = 1.0f;
    float mGreen = 1.0f;
    float mBlue = 1.0f;
    float mAlpha = 1.0f;
};

static constexpr float kDrawColorRed[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr float kDrawColorGreen[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

class CombinedFramebufferBlit : public ::testing::Test, public ::testing::WithParamInterface<ClearColorParam> {
protected:
    virtual void SetUp() override {
        mApp.reset(new ClearColor(GetParam()));
    }

    virtual void TearDown() override {
        mApp.reset();
        EXPECT_EQ(EGL_SUCCESS, LazyLoadedEGLDispatch::get()->eglGetError())
                << "DefaultFramebufferBlitTest TearDown found an EGL error";
    }

    std::unique_ptr<ClearColor> mApp;
};

TEST_P(CombinedFramebufferBlit, DefaultDrawDefaultRead) {
    // Draw to default framebuffer; color should show up
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);
}

TEST_P(CombinedFramebufferBlit, NonDefault) {
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);

    // Color should not be affected by the draw to a non-default framebuffer.
    mApp->setNonDefaultCombinedFbo();

    mApp->drawWithColor(kDrawColorGreen);
    mApp->verifySwappedColor(kDrawColorRed);
}

// Test blitting both with and without the fast blit path.
INSTANTIATE_TEST_CASE_P(CombinedFramebufferBlitTest,
                        CombinedFramebufferBlit,
                        testing::Values(
                            ClearColorParam(GLESApi_CM, true),
                            ClearColorParam(GLESApi_CM, false),
                            ClearColorParam(GLESApi_2, true),
                            ClearColorParam(GLESApi_2, false),
                            ClearColorParam(GLESApi_3_0, true),
                            ClearColorParam(GLESApi_3_0, false)));

class NonDefaultFramebufferBlit : public CombinedFramebufferBlit { };

TEST_P(NonDefaultFramebufferBlit, NonDefaultDrawNonDefaultRead) {
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

TEST_P(NonDefaultFramebufferBlit, DefaultDrawNonDefaultRead) {
    mApp->drawWithColor(kDrawColorRed);
    mApp->verifySwappedColor(kDrawColorRed);

    // Bug: 110996998
    // Draw to default framebuffer, and have a non-default
    // read framebuffer bound. Color should show up
    mApp->setNonDefaultReadFbo();

    mApp->drawWithColor(kDrawColorGreen);
    mApp->verifySwappedColor(kDrawColorGreen);
}

TEST_P(NonDefaultFramebufferBlit, NonDefaultDrawDefaultRead) {
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

// Test blitting both with and without the fast blit path.
INSTANTIATE_TEST_CASE_P(DefaultFramebufferBlitTest,
                        NonDefaultFramebufferBlit,
                        testing::Values(
                            ClearColorParam(GLESApi_3_0, true),
                            ClearColorParam(GLESApi_3_0, false)));


}  // namespace emugl
