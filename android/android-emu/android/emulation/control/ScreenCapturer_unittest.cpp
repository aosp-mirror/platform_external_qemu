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

#include "OpenglRender/RenderChannel.h"
#include "OpenglRender/Renderer.h"
#include "android/base/Compiler.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "observation.pb.h"
#include "android/loadpng.h"

#include <cstdio>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using android::base::System;
using android::base::PathUtils;
using android::emulation::captureScreenshot;
using android::emulation::takeScreenshot;
using android::emulation::ImageFormat;
using android::emulation::Image;
using ::testing::Gt;

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

    static void verifyPixels(SkinRotation rotation,
                             const uint8_t* data,
                             const uint8_t* pixels_0,
                             const uint8_t* pixels_90,
                             const uint8_t* pixels_180,
                             const uint8_t* pixels_270,
                             size_t bufsize) {
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
        for (int i = 0; i < bufsize; i++) {
            EXPECT_EQ(pixels[i], data[i])
                    << std::string("Bad value at position ") +
                               std::to_string(i);
        }
    }

    static void verifyframebuffer4Pixels(SkinRotation rotation, const uint8_t* data) {
        static const uint8_t pixels_0[]   = {1, 0, 0, 255,    0, 1, 0, 255,
                                             0, 0, 1, 255,    1, 1, 1, 255};
        static const uint8_t pixels_90[]  = {0, 0, 1, 255,    1, 0, 0, 255,
                                             1, 1, 1, 255,    0, 1, 0, 255};
        static const uint8_t pixels_180[] = {1, 1, 1, 255,    0, 0, 1, 255,
                                             0, 1, 0, 255,    1, 0, 0, 255};
        static const uint8_t pixels_270[] = {0, 1, 0, 255,    1, 1, 1, 255,
                                             1, 0, 0, 255,    0, 0, 1, 255};

        verifyPixels(rotation ,data, pixels_0, pixels_90, pixels_180, pixels_270, sizeof(pixels_0));
    }

    static void verifyframebuffer3Pixels(SkinRotation rotation, const uint8_t* data) {
        static const uint8_t pixels_0[]   = {1, 0, 0,    0, 1, 0,
                                             0, 0, 1,    1, 1, 1};
        static const uint8_t pixels_90[]  = {0, 0, 1,    1, 0, 0,
                                             1, 1, 1,    0, 1, 0};
        static const uint8_t pixels_180[] = {1, 1, 1,    0, 0, 1,
                                             0, 1, 0,    1, 0, 0};
        static const uint8_t pixels_270[] = {0, 1, 0,    1, 1, 1,
                                             1, 0, 0,    0, 0, 1};

        verifyPixels(rotation, data, pixels_0, pixels_90, pixels_180, pixels_270, sizeof(pixels_0));
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

    void* addressSpaceGraphicsConsumerCreate(
        struct asg_context,
        android::emulation::asg::ConsumerCallbacks) {
        return nullptr;
    }

    void addressSpaceGraphicsConsumerDestroy(void*) { }

    HardwareStrings getHardwareStrings() {
        return {};
    }
    void setPostCallback(OnPostCallback onPost, void* context, bool useBgraReadback, uint32_t displayId) {
        return;
    }
    bool asyncReadbackSupported() {
        return false;
    }
    ReadPixelsCallback getReadPixelsCallback() {
        return nullptr;
    }

    FlushReadPixelPipeline getFlushReadPixelPipeline()  {
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
                                     bool deleteExisting,
                                     bool hideWindow) {
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
    void setMultiDisplay(uint32_t id,
                         int32_t x,
                         int32_t y,
                         uint32_t w,
                         uint32_t h,
                         uint32_t dpi,
                         bool add) { }
    void setMultiDisplayColorBuffer(uint32_t id, uint32_t cb) { }

    void cleanupProcGLObjects(uint64_t puid) { }
    struct AndroidVirtioGpuOps* getVirtioGpuOps() { return nullptr; }
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
        unsigned int* height, std::vector<unsigned char>& pixels,
        int displayId, int desiredWidth, int desiredHeight,
        SkinRotation desiredRotation) {
        if (mHasValidScreenshot) {
            if (desiredWidth == 0 && desiredHeight == 0) {
                *width = kWidth;
                *height = kHeight;
                pixels.assign(*width * *height * nChannels, 0);
            } else {
                *width = desiredWidth;
                *height = desiredHeight;
                pixels.assign(*width * *height * nChannels, 0);
            }
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

TEST_F(ScreenCapturerTest, takePngScreenShotRotations) {
    // Make sure that taking a screenshot that gets converted in place
    // to a png actually works.
    const SkinRotation rotations[] = {SKIN_ROTATION_0, SKIN_ROTATION_90,
                                      SKIN_ROTATION_180, SKIN_ROTATION_270};

    std::string screenShot = PathUtils::join(mScreenshotPath, "png_capture.png");
    for (SkinRotation rotation : rotations) {
        Image img = takeScreenshot(ImageFormat::PNG, rotation, nullptr,
                                   framebuffer4Pixels);

        // Let's convert this image into a set of pixels.
        std::ofstream ofile(screenShot, std::ofstream::binary | std::ofstream::trunc);
        ofile.write((char*) img.getPixelBuf(), img.getPixelCount());
        ofile.flush();
        ofile.close();
        uint8_t* pixels = (uint8_t*)loadScreenshot(screenShot.c_str(), 2, 2);

        verifyframebuffer4Pixels(rotation, pixels);
        free(pixels);
    }
}

TEST_F(ScreenCapturerTest, takeRGB888ScreenShotRotations) {
    // Make sure that taking a screenshot that gets converted in place
    // to a png actually works.
    const SkinRotation rotations[] = {SKIN_ROTATION_0};

    // Currently we do not do software rotation.
    for (SkinRotation rotation : rotations) {
        Image img = takeScreenshot(ImageFormat::RGB888, rotation, nullptr,
                                   framebuffer4Pixels);
        EXPECT_EQ(ImageFormat::RGB888, img.getImageFormat());

        verifyframebuffer3Pixels(rotation, img.getPixelBuf());

        // Nop conversion should not modify the pixel buffers.
        verifyframebuffer3Pixels(rotation, img.asRGB888().getPixelBuf());
    }
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

TEST_F(ScreenCapturerTest, pbFileSuccess) {
    MockRenderer renderer(true);
    // Create a temporary file with a .pb extension.

    const std::string tmp_file = PathUtils::join(
                mTestSystem.getTempRoot()->path(), "normal_file.pb");
    FILE* file = std::fopen(tmp_file.c_str(), "w");
    EXPECT_THAT(file, testing::NotNull()) << "Failed to open " << tmp_file;
    EXPECT_THAT(std::fclose(file), testing::Eq(0));

    // Capture the screenshot.
    EXPECT_TRUE(captureScreenshot(&renderer, nullptr, SKIN_ROTATION_0,
                                  tmp_file.c_str()));

    // Check that the file is not empty and contains a serialized Observation.
    std::ifstream infile(tmp_file.c_str(), std::ios::binary);
    EXPECT_FALSE(!infile) << "Failed to open " << tmp_file;

    android::emulation::Observation observation;
    EXPECT_TRUE(observation.ParseFromIstream(&infile))
            << "Failed to parse Observation proto";
    EXPECT_THAT(observation.ByteSize(), Gt(0));
    const android::emulation::Observation::Image& image = observation.screen();
    EXPECT_THAT(image.width(), Gt(0));
    EXPECT_THAT(image.height(), Gt(0));
    EXPECT_THAT(image.num_channels(), Gt(0));
    EXPECT_THAT(image.data(), testing::Not(testing::IsEmpty()));

    remove(tmp_file.c_str());  // Cleanup temporary file.
}

TEST_F(ScreenCapturerTest, scaleSuccess) {
    MockRenderer renderer(true);

    // Capture the screenshot.
    Image image = takeScreenshot(ImageFormat::RAW, SKIN_ROTATION_0, &renderer, nullptr,
                                 0, 600, 800);
    EXPECT_TRUE(image.getWidth() == 600);
    EXPECT_TRUE(image.getHeight() == 800);
}
