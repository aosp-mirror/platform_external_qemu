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

#include "android/base/files/PathUtils.h"
#include "android/base/memory/OnDemand.h"
#include "android/base/system/System.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/emugl_config.h"

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"
#include "AndroidWindowBuffer.h"
#include "ClientComposer.h"
#include "Display.h"
#include "GrallocDispatch.h"

#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <string>

#include <stdio.h>

using aemu::AndroidBufferQueue;
using aemu::AndroidWindow;
using aemu::AndroidWindowBuffer;
using aemu::ClientComposer;
using aemu::Display;

using android::AndroidPipe;
using android::base::makeOnDemand;
using android::base::OnDemandT;
using android::base::pj;
using android::base::System;
using android::HostGoldfishPipeDevice;

static constexpr int kWindowSize = 256;

// The test environment configures the host-side render lib.
// It will live for the duration of all tests.
class GoldfishOpenglTestEnv {
public:
    GoldfishOpenglTestEnv() {
        System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR", System::get()->getProgramDirectory());
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLESDynamicVersion, false);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLDMA, false);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLAsyncSwap, false);

        EmuglConfig config;

        emuglConfig_init(&config, true /* gpu enabled */, "auto",
                         "swiftshader_indirect", /* gpu mode, option */
                         64,                     /* bitness */
                         true,                   /* no window */
                         false,                  /* blacklisted */
                         false,                  /* has guest renderer */
                         WINSYS_GLESBACKEND_PREFERENCE_AUTO);

        emuglConfig_setupEnv(&config);

        android_initOpenglesEmulation();

        int maj;
        int min;

        android_startOpenglesRenderer(
            kWindowSize, kWindowSize, 1, 28, &maj, &min);

        char* vendor = nullptr;
        char* renderer = nullptr;
        char* version = nullptr;

        android_getOpenglesHardwareStrings(
            &vendor, &renderer, &version);

        printf("%s: GL strings; [%s] [%s] [%s].\n", __func__, vendor, renderer,
               version);

        HostGoldfishPipeDevice::get();

        android_init_opengles_pipe();
    }

    ~GoldfishOpenglTestEnv() {
        AndroidPipe::Service::resetAll();
        android_stopOpenglesRenderer(true);
    }
};


class CombinedGoldfishOpenglTest : public ::testing::Test {
protected:

    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        testEnv = new GoldfishOpenglTestEnv;
    }

    static void TearDownTestCase() {
        delete testEnv;
        testEnv = nullptr;
    }

    struct EGLState {
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLConfig config;
        EGLSurface surface = EGL_NO_SURFACE;
        EGLContext context = EGL_NO_CONTEXT;
    };

    void SetUp() override {
        setupGralloc();
        setupEGLAndMakeCurrent();
    }

    void TearDown() override {
        teardownEGL();
        teardownGralloc();
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);

        EXPECT_NE(nullptr, mGralloc.fb_dev);
        EXPECT_NE(nullptr, mGralloc.alloc_dev);
        EXPECT_NE(nullptr, mGralloc.fb_module);
        EXPECT_NE(nullptr, mGralloc.alloc_module);
    }

    void teardownGralloc() { unload_gralloc_module(&mGralloc); }

    void setupEGLAndMakeCurrent() {
        int maj, min;

        mEGL.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EXPECT_EQ(EGL_TRUE, eglInitialize(mEGL.display, &maj, &min));

        eglBindAPI(EGL_OPENGL_ES_API);

        static const EGLint configAttribs[] = {
                EGL_SURFACE_TYPE,   EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                EGL_OPENGL_ES2_BIT, EGL_NONE,
        };

        int numConfigs;

        EXPECT_EQ(EGL_TRUE, eglChooseConfig(mEGL.display, configAttribs,
                                            &mEGL.config, 1, &numConfigs))
                << "Could not find GLES2 config!";

        EXPECT_GT(numConfigs, 0);

        static const EGLint pbufAttribs[] = {EGL_WIDTH, kWindowSize, EGL_HEIGHT,
                                             kWindowSize, EGL_NONE};

        mEGL.surface =
                eglCreatePbufferSurface(mEGL.display, mEGL.config, pbufAttribs);

        EXPECT_NE(EGL_NO_SURFACE, mEGL.surface)
                << "Could not create pbuffer surface!";

        static const EGLint contextAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION,
                2,
                EGL_NONE,
        };

        mEGL.context = eglCreateContext(mEGL.display, mEGL.config,
                                        EGL_NO_CONTEXT, contextAttribs);

        EXPECT_NE(EGL_NO_CONTEXT, mEGL.context) << "Could not create context!";

        EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, mEGL.surface,
                                           mEGL.surface, mEGL.context))
                << "eglMakeCurrent failed!";
    }

    void teardownEGL() {
        EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, EGL_NO_SURFACE,
                                           EGL_NO_SURFACE, EGL_NO_CONTEXT));
        EXPECT_EQ(EGL_TRUE, eglDestroyContext(mEGL.display, mEGL.context));
        EXPECT_EQ(EGL_TRUE, eglDestroySurface(mEGL.display, mEGL.surface));
        EXPECT_EQ(EGL_TRUE, eglTerminate(mEGL.display));
        EXPECT_EQ(EGL_TRUE, eglReleaseThread());

        // Cancel all host threads as well
        android_finishOpenglesRenderer();
    }

    buffer_handle_t createTestGrallocBuffer(
            int usage = GRALLOC_USAGE_HW_RENDER,
            int format = HAL_PIXEL_FORMAT_RGBA_8888,
            int width = kWindowSize,
            int height = kWindowSize) {
        buffer_handle_t buffer;
        int stride;

        mGralloc.alloc(width, height, format, usage, &buffer, &stride);
        mGralloc.registerBuffer(buffer);

        (void)stride;

        return buffer;
    }

    void destroyTestGrallocBuffer(buffer_handle_t buffer) {
        mGralloc.unregisterBuffer(buffer);
        mGralloc.free(buffer);
    }

    std::vector<AndroidWindowBuffer> primeWindowBufferQueue(AndroidWindow& window) {
        EXPECT_NE(nullptr, window.fromProducer);
        EXPECT_NE(nullptr, window.toProducer);

        std::vector<AndroidWindowBuffer> buffers;
        for (int i = 0; i < AndroidBufferQueue::kCapacity; i++) {
            buffers.push_back(AndroidWindowBuffer(
                    &window, createTestGrallocBuffer()));
        }

        for (int i = 0; i < AndroidBufferQueue::kCapacity; i++) {
            window.fromProducer->queueBuffer(AndroidBufferQueue::Item(
                    (ANativeWindowBuffer*)(buffers.data() + i)));
        }
        return buffers;
    }

    class WindowWithBacking {
    public:
        WindowWithBacking(CombinedGoldfishOpenglTest* _env)
            : env(_env), window(kWindowSize, kWindowSize) {
            window.setProducer(&toWindow, &fromWindow);
            buffers = env->primeWindowBufferQueue(window);
        }

        ~WindowWithBacking() {
            for (auto& buffer : buffers) {
                env->destroyTestGrallocBuffer(buffer.handle);
            }
        }

        CombinedGoldfishOpenglTest* env;
        AndroidWindow window;
        AndroidBufferQueue toWindow;
        AndroidBufferQueue fromWindow;
        std::vector<AndroidWindowBuffer> buffers;
    };

    struct gralloc_implementation mGralloc;
    EGLState mEGL;
};

// static
GoldfishOpenglTestEnv* CombinedGoldfishOpenglTest::testEnv = nullptr;

// Tests that the pipe connection can be made and that
// gralloc/EGL can be set up.
TEST_F(CombinedGoldfishOpenglTest, Basic) { }

// Tests allocation and deletion of a gralloc buffer.
TEST_F(CombinedGoldfishOpenglTest, GrallocBuffer) {
    buffer_handle_t buffer = createTestGrallocBuffer();
    destroyTestGrallocBuffer(buffer);
}

// Tests basic GLES usage.
TEST_F(CombinedGoldfishOpenglTest, ViewportQuery) {
    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);

    EXPECT_EQ(0, viewport[0]);
    EXPECT_EQ(0, viewport[1]);
    EXPECT_EQ(kWindowSize, viewport[2]);
    EXPECT_EQ(kWindowSize, viewport[3]);
}

// Tests eglCreateWindowSurface.
TEST_F(CombinedGoldfishOpenglTest, CreateWindowSurface) {
    AndroidWindow testWindow(kWindowSize, kWindowSize);

    AndroidBufferQueue toWindow;
    AndroidBufferQueue fromWindow;

    testWindow.setProducer(&toWindow, &fromWindow);

    AndroidWindowBuffer testBuffer(&testWindow, createTestGrallocBuffer());

    toWindow.queueBuffer(AndroidBufferQueue::Item(&testBuffer));

    static const EGLint attribs[] = {EGL_WIDTH, kWindowSize, EGL_HEIGHT,
                                     kWindowSize, EGL_NONE};
    EGLSurface testEGLWindow =
            eglCreateWindowSurface(mEGL.display, mEGL.config,
                                   (EGLNativeWindowType)(&testWindow), attribs);

    EXPECT_NE(EGL_NO_SURFACE, testEGLWindow);
    EXPECT_EQ(EGL_TRUE, eglDestroySurface(mEGL.display, testEGLWindow));

    destroyTestGrallocBuffer(testBuffer.handle);
}

// Starts a client composer, but doesn't do anything with it.
TEST_F(CombinedGoldfishOpenglTest, ClientCompositionSetup) {

    WindowWithBacking composeWindow(this);
    WindowWithBacking appWindow(this);

    ClientComposer composer(&composeWindow.window, &appWindow.fromWindow,
                            &appWindow.toWindow);
}

// Client composition, but also advances a frame.
TEST_F(CombinedGoldfishOpenglTest, ClientCompositionAdvanceFrame) {

    WindowWithBacking composeWindow(this);
    WindowWithBacking appWindow(this);

    AndroidBufferQueue::Item item;

    appWindow.toWindow.dequeueBuffer(&item);
    appWindow.fromWindow.queueBuffer(item);

    ClientComposer composer(&composeWindow.window, &appWindow.fromWindow,
                            &appWindow.toWindow);

    composer.advanceFrame();
    composer.join();
}

TEST_F(CombinedGoldfishOpenglTest, ShowWindow) {
    bool useWindow =
            System::get()->envGet("ANDROID_EMU_TEST_WITH_WINDOW") == "1";

    aemu::Display disp(useWindow, kWindowSize, kWindowSize);

    if (useWindow) {
        EXPECT_NE(nullptr, disp.getNative());
        android_showOpenglesWindow(disp.getNative(), 0, 0, kWindowSize,
                                   kWindowSize, kWindowSize, kWindowSize, 1.0,
                                   0, false);
    }

    for (int i = 0; i < 10; i++) {
        disp.loop();
        System::get()->sleepMs(1);
    }

    if (useWindow) android_hideOpenglesWindow();
}
