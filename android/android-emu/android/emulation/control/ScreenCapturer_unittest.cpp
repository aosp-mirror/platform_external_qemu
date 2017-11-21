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
#include "OpenglRender/RenderChannel.h"
#include "OpenglRender/Renderer.h"

#include <gtest/gtest.h>
#include <string>

using android::base::System;
using android::emulation::captureScreenshot;

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
    void cleanupProcGLObjects(uint64_t puid) { }
    void stop(bool wait) { }
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
private:
    bool mHasValidScreenshot = false;
    static const int kWidth = 1080;
    static const int kHeight = 1920;
};

TEST_F(ScreenCapturerTest, badRenderer) {
    EXPECT_FALSE(captureScreenshot(nullptr, mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, rendererCaptureFailure) {
    MockRenderer renderer(false);
    EXPECT_FALSE(captureScreenshot(&renderer, mScreenshotPath.c_str()));
}

TEST_F(ScreenCapturerTest, success) {
    MockRenderer renderer(true);
    EXPECT_TRUE(captureScreenshot(&renderer, mScreenshotPath.c_str()));
}
