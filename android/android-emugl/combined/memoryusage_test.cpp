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

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"
#include "AndroidWindowBuffer.h"
#include "ClientComposer.h"
#include "Display.h"
#include "GoldfishOpenglTestEnv.h"
#include "GrallocDispatch.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/opengl_object_counter.h"
#include "android/opengles.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <hardware/gralloc.h>

#include <string>

#include <stdio.h>

using aemu::AndroidBufferQueue;
using aemu::AndroidWindow;
using aemu::AndroidWindowBuffer;
using aemu::ClientComposer;
using aemu::Display;

using android::base::FunctorThread;
using android::base::pj;
using android::base::System;

static constexpr int kWindowSize = 256;

class MemoryUsageTest : public ::testing::Test {
protected:
    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::RefCountPipe, true);
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
        if (mEGL.display != EGL_NO_DISPLAY) {
            EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, EGL_NO_SURFACE,
                                               EGL_NO_SURFACE, EGL_NO_CONTEXT));
            EXPECT_EQ(EGL_TRUE, eglDestroyContext(mEGL.display, mEGL.context));
            EXPECT_EQ(EGL_TRUE, eglDestroySurface(mEGL.display, mEGL.surface));
            EXPECT_EQ(EGL_TRUE, eglTerminate(mEGL.display));
            EXPECT_EQ(EGL_TRUE, eglReleaseThread());
        }
        // Cancel all host threads as well
        android_finishOpenglesRenderer();
    }

    void SetUp() override { setupEGLAndMakeCurrent(); }

    void TearDown() override { teardownEGL(); }

    EGLState mEGL;

    void eglRelease() {
        if (mEGL.display != EGL_NO_DISPLAY) {
            eglMakeCurrent(mEGL.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                           EGL_NO_CONTEXT);
            eglDestroyContext(mEGL.display, mEGL.context);
            eglDestroySurface(mEGL.display, mEGL.surface);
            eglTerminate(mEGL.display);
            eglReleaseThread();
        }

        mEGL.display = EGL_NO_DISPLAY;
        mEGL.context = EGL_NO_CONTEXT;
        mEGL.surface = EGL_NO_SURFACE;
    }
};

// static
GoldfishOpenglTestEnv* MemoryUsageTest::testEnv = nullptr;

TEST_F(MemoryUsageTest, ThreadCleanup) {
    // initial clean up.
    eglRelease();
    std::vector<int> beforeTest = get_opengl_object_counts();
    for (int i = 0; i < 1000; i++) {
        std::unique_ptr<FunctorThread> th(new FunctorThread([this]() {
            setupEGLAndMakeCurrent();
            eglRelease();
        }));
        ;
        th->start();
        th->wait();
    }
    std::vector<int> afterTest = get_opengl_object_counts();
    for (int i = 0; i < beforeTest.size(); i++) {
        EXPECT_TRUE(beforeTest[i] >= afterTest[i]);
    }
}
