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

#include "Toplevel.h"

#include "android/base/GLObjectCounter.h"
#include "android/base/system/System.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <memory>

using android::base::System;

namespace aemu {

TEST(Toplevel, Basic) {
    std::vector<size_t> beforeTest, afterTest;
    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR", System::get()->getProgramDirectory());
    auto t(std::make_unique<Toplevel>());

    // When framebuffer is finalized, not all gl resources will be released even
    // when Toplevel object is destroyed. The workaround is to get the gl object
    // counts after the Toplevel is created.
    beforeTest = android::base::GLObjectCounter::get()->getCounts();

    auto win = t->createWindow();
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min;
    eglInitialize(d, &maj, &min);

    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint total_num_configs = 0;
    eglGetConfigs(d, 0, 0, &total_num_configs);

    EGLint wantedRedSize = 8;
    EGLint wantedGreenSize = 8;
    EGLint wantedBlueSize = 8;

    const GLint configAttribs[] = {
            EGL_RED_SIZE,       wantedRedSize,
            EGL_GREEN_SIZE, wantedGreenSize,    EGL_BLUE_SIZE, wantedBlueSize,
            EGL_SURFACE_TYPE,   EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLint total_compatible = 0;

    std::vector<EGLConfig> configs(total_num_configs);
    eglChooseConfig(d, configAttribs, configs.data(), total_num_configs, &total_compatible);

    EGLint exact_match_index = -1;
    for (EGLint i = 0; i < total_compatible; i++) {
        EGLint r, g, b;
        EGLConfig c = configs[i];
        eglGetConfigAttrib(d, c, EGL_RED_SIZE, &r);
        eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, &g);
        eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, &b);

        if (r == wantedRedSize && g == wantedGreenSize && b == wantedBlueSize) {
            exact_match_index = i;
            break;
        }
    }

    EGLConfig match = configs[exact_match_index];

    static const GLint glesAtt[] =
    { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    EGLContext c = eglCreateContext(d, match, EGL_NO_CONTEXT, glesAtt);

    EGLSurface s = eglCreateWindowSurface(d, match, win, 0);

    eglMakeCurrent(d, s, s, c);
    for (int i = 0; i < 120; i++) {
        glViewport(0, 0, 256, 256);
        glClearColor(1.0f, 0.0f, i / 120.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glFinish();
        eglSwapBuffers(d, s);
        t->loop();
    }
    EXPECT_EQ(EGL_TRUE, eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                       EGL_NO_CONTEXT));
    EXPECT_EQ(EGL_TRUE, eglDestroyContext(d, c));
    EXPECT_EQ(EGL_TRUE, eglDestroySurface(d, s));
    // Note: only necessary to ensure color buffers swept
    EXPECT_EQ(EGL_TRUE, eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                       EGL_NO_CONTEXT));
    eglReleaseThread();

    t.reset();

    afterTest = android::base::GLObjectCounter::get()->getCounts();
    for (int i = 0; i < beforeTest.size(); i++) {
        EXPECT_TRUE(beforeTest[i] >= afterTest[i]);
    }
}

}  // namespace aemu
