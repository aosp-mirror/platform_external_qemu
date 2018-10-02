// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/ScreenCapturer.h"

#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/loadpng.h"
#include "OpenglRender/RenderChannel.h"
#include "OpenglRender/Renderer.h"

#include <gtest/gtest.h>
#include <string>

using android::base::System;
using android::emulation::captureScreenshot;

extern "C" EmulatorWindow* emulator_window_get(void) {
    return NULL;
}

extern const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent;

class ScreenCapturerTest : public ::testing::Test {
public:
    ScreenCapturerTest()
        : mTestSystem(PATH_SEP "progdir",
                      System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {}

    void SetUp() override {
        mTestSystem.getTempRoot()->makeSubDir(PATH_SEP "ScreencapOut");
        mScreenshotPath = mTestSystem.getTempRoot()->makeSubPath(
                PATH_SEP "ScreencapOut");
    }
protected:
    DISALLOW_COPY_AND_ASSIGN(ScreenCapturerTest);
    android::base::TestSystem mTestSystem;
    std::string mScreenshotPath = {};
    void* loadScreenshot(const char* fileName, unsigned expectW,
            unsigned expectH) {
        unsigned w = 0;
        unsigned h = 0;
        void* ret = loadpng(fileName, &w, &h);
        EXPECT_EQ(expectW, w);
        EXPECT_EQ(expectH, h);
        return ret;
    }
    static void framebuffer4Pixels(int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
        static uint8_t pixels[] = {1, 0, 0, 255,    0, 1, 0, 255,
                                   0, 0, 1, 255,    1, 1, 1, 255};
        *w = 2;
        *h = 2;
        *bytesPerPixel = 4;
        *lineSize = *w * *bytesPerPixel;
        *frameBufferData = pixels;
    }
    static void verifyframebuffer4Pixels(SkinRotation rotation,
            const uint8_t* data) {
        static const uint8_t pixels_0[]   = {1, 0, 0, 255,    0, 1, 0, 255,
                                             0, 0, 1, 255,    1, 1, 1, 255};
        static const uint8_t pixels_90[]  = {0, 0, 1, 255,    1, 0, 0, 255,
                                             1, 1, 1, 255,    0, 1, 0, 255};
        static const uint8_t pixels_180[] = {1, 1, 1, 255,    0, 0, 1, 255,
                                             0, 1, 0, 255,    1, 0, 0, 255};
        static const uint8_t pixels_270[] = {0, 1, 0, 255,    1, 1, 1, 255,
                                             1, 0, 0, 255,    0, 0, 1, 255};
        const uint8_t* pixels = nullptr;
        switch (rotation) {
            case SKIN_ROTATION_0:
                pixels = pixels_0;
                break;
            case SKIN_ROTATION_90:
                pixels = pixels_90;
                break;
            case SKIN_ROTATION_180:
                pixels = pixels_180;
                break;
            case SKIN_ROTATION_270:
                pixels = pixels_270;
                break;
        }
        for (int i = 0; i < sizeof(pixels_0); i++) {
            EXPECT_EQ(pixels[i], data[i])
                    << std::string("Bad value at position ")
                        + std::to_string(i);
        }
    }
};

class MockRenderer : public emugl::Renderer {
public:
    MockRenderer(bool hasValidScreenshot)
        : mHasValidScreenshot(hasValidScreenshot) { }
    emugl::RenderChannelPtr createRenderChannel(
            android::base::Stream* loadStream = nullptr) {
        return nullptr;
    }
    HardwareStrings getHardwareStrings() {
        return {};
    }
    void setPostCallback(OnPostCallback onPost, void* context) {
        return;
    }
    bool asyncReadbackSupported() {
        return false;
    }
    ReadPixelsCallback getReadPixelsCallback() {
        return nullptr;
    }
    bool showOpenGLSubwindow(FBNativeWindowType window,
                                     int wx,
                                     int wy,
                                     int ww,
                                     int wh,
                                     int fbw,
                                     int fbh,
                                     float dpr,
                                     float zRot,
                                     bool deleteExisting) {
        return false;
    }
    bool destroyOpenGLSubwindow() {
        return false;
    }
    void setOpenGLDisplayRotation(float zRot) { }
    void setOpenGLDisplayTranslation(float px, float py) { }
    void repaintOpenGLDisplay() { }

    bool hasGuestPostedAFrame() {
        return mGuestPostedAFrame;
    }

    void resetGuestPostedAFrame() {
        mGuestPostedAFrame = false;
    }

    void setGuestPostedAFrame() {
        mGuestPostedAFrame = true;
    }

    void setScreenMask(int width, int height, const unsigned char* rgbaData) { }
    void cleanupProcGLObjects(uint64_t puid) { }
    void stop(bool wait) { }
    void finish() { }
    void pauseAllPreSave() { }
    void resumeAll() { }
    void save(android::base::Stream* stream,
            const android::snapshot::ITextureSaverPtr& textureSaver) { }
    bool load(android::base::Stream* stream,
            const android::snapshot::ITextureLoaderPtr& textureLoader) {
        return false;
    }
    void fillGLESUsages(android_studio::EmulatorGLESUsages*) { }
    void getScreenshot(unsigned int nChannels, unsigned int* width,
        unsigned int* height, std::vector<unsigned char>& pixels) {
        if (mHasValidScreenshot) {
            *width = kWidth;
            *height = kHeight;
            pixels.assign(*width * *height * nChannels, 0);
        } else {
            *width = 0;
            *height = 0;
            pixels.resize(0);
        }
    }
    void snapshotOperationCallback(
            android::snapshot::Snapshotter::Operation op,
            android::snapshot::Snapshotter::Stage stage) {}
private:
    bool mHasValidScreenshot = false;
    bool mGuestPostedAFrame = false;
    static const int kWidth = 1080;
    static const int kHeight = 1920;
};

TEST_F(ScreenCapturerTest, badRenderer) {
    EXPECT_FALSE(captureScreenshot(nullptr, nullptr, SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, rendererCaptureFailure) {
    MockRenderer renderer(false);
    EXPECT_FALSE(captureScreenshot(&renderer, nullptr, SKIN_ROTATION_0,
    mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, success) {
    MockRenderer renderer(true);
    EXPECT_TRUE(captureScreenshot(&renderer, nullptr, SKIN_ROTATION_0,
    mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, badGetFrameBufferWidth) {
    EXPECT_FALSE(captureScreenshot(nullptr,
            [](int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
                *w = 0;
            },
            SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, badGetFrameBufferBpp) {
    EXPECT_FALSE(captureScreenshot(nullptr,
            [](int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
                *bytesPerPixel = 0;
            },
            SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, getFrameBufferRgb565) {
    uint8_t buffer[2] = {0, 0};
    EXPECT_TRUE(captureScreenshot(nullptr,
            [&buffer](int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
                *w = 1;
                *h = 1;
                *bytesPerPixel = 2;
                *frameBufferData = buffer;
            },
            SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, getFrameBufferRgb888) {
    uint8_t buffer[3] = {0, 0, 0};
    EXPECT_TRUE(captureScreenshot(nullptr,
            [&buffer](int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
                *w = 1;
                *h = 1;
                *bytesPerPixel = 3;
                *frameBufferData = buffer;
            },
            SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, getFrameBufferRgba888) {
    uint8_t buffer[4] = {0, 0, 0, 0};
    EXPECT_TRUE(captureScreenshot(nullptr,
            [&buffer](int* w, int* h, int* lineSize,
               int* bytesPerPixel, uint8_t** frameBufferData) {
                *w = 1;
                *h = 1;
                *bytesPerPixel = 4;
                *frameBufferData = buffer;
            },
            SKIN_ROTATION_0,
            mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, guestPostedAFrame) {
    MockRenderer renderer(false);

    EXPECT_FALSE(renderer.hasGuestPostedAFrame());

    renderer.setGuestPostedAFrame();

    EXPECT_TRUE(renderer.hasGuestPostedAFrame());

    renderer.resetGuestPostedAFrame();

    EXPECT_FALSE(renderer.hasGuestPostedAFrame());
}

TEST_F(ScreenCapturerTest, saveAndLoadRotation0) {
    std::string screenshotName;
    EXPECT_TRUE(captureScreenshot(nullptr,
                framebuffer4Pixels,
                SKIN_ROTATION_0,
                mScreenshotPath.c_str(),
                &screenshotName));
    uint8_t* pixels = (uint8_t*)loadScreenshot(screenshotName.c_str(), 2, 2);
    verifyframebuffer4Pixels(SKIN_ROTATION_0, pixels);
    free(pixels);
}

TEST_F(ScreenCapturerTest, saveAndLoadRotation90) {
    std::string screenshotName;
    EXPECT_TRUE(captureScreenshot(nullptr,
                framebuffer4Pixels,
                SKIN_ROTATION_90,
                mScreenshotPath.c_str(),
                &screenshotName));
    uint8_t* pixels = (uint8_t*)loadScreenshot(screenshotName.c_str(), 2, 2);
    verifyframebuffer4Pixels(SKIN_ROTATION_90, pixels);
    free(pixels);
}

TEST_F(ScreenCapturerTest, saveAndLoadRotation180) {
    std::string screenshotName;
    EXPECT_TRUE(captureScreenshot(nullptr,
                framebuffer4Pixels,
                SKIN_ROTATION_180,
                mScreenshotPath.c_str(),
                &screenshotName));
    uint8_t* pixels = (uint8_t*)loadScreenshot(screenshotName.c_str(), 2, 2);
    verifyframebuffer4Pixels(SKIN_ROTATION_180, pixels);
    free(pixels);
}

TEST_F(ScreenCapturerTest, saveAndLoadRotation270) {
    std::string screenshotName;
    EXPECT_TRUE(captureScreenshot(nullptr,
                framebuffer4Pixels,
                SKIN_ROTATION_270,
                mScreenshotPath.c_str(),
                &screenshotName));
    uint8_t* pixels = (uint8_t*)loadScreenshot(screenshotName.c_str(), 2, 2);
    verifyframebuffer4Pixels(SKIN_ROTATION_270, pixels);
    free(pixels);
}
