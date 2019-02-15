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

#include "android/base/system/System.h"
#include "android/opengl/GLObjectCounter.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <memory>

using android::base::System;

namespace aemu {

TEST(Toplevel, Basic) {

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    std::vector<int> beforeTest, afterTest;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR", System::get()->getProgramDirectory());
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    auto t(std::make_unique<Toplevel>());
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    // When framebuffer is finalized, not all gl resources will be released even
    // when Toplevel object is destroyed. The workaround is to get the gl object
    // counts after the Toplevel is created.
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    android::opengl::getOpenGLObjectCounts(&beforeTest);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    auto win = t->createWindow();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EGLint maj, min;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglInitialize(d, &maj, &min);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglBindAPI(EGL_OPENGL_ES_API);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EGLint total_num_configs = 0;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglGetConfigs(d, 0, 0, &total_num_configs);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    EGLint wantedRedSize = 8;
    EGLint wantedGreenSize = 8;
    EGLint wantedBlueSize = 8;

    const GLint configAttribs[] = {
            EGL_RED_SIZE,       wantedRedSize,
            EGL_GREEN_SIZE, wantedGreenSize,    EGL_BLUE_SIZE, wantedBlueSize,
            EGL_SURFACE_TYPE,   EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLint total_compatible = 0;

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    std::vector<EGLConfig> configs(total_num_configs);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglChooseConfig(d, configAttribs, configs.data(), total_num_configs, &total_compatible);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    EGLint exact_match_index = -1;
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    for (EGLint i = 0; i < total_compatible; i++) {
    fprintf(stderr, "%s:%d arrive %d\n", __func__, __LINE__, i);
        EGLint r, g, b;
        EGLConfig c = configs[i];
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        eglGetConfigAttrib(d, c, EGL_RED_SIZE, &r);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, &g);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, &b);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

        if (r == wantedRedSize && g == wantedGreenSize && b == wantedBlueSize) {
            exact_match_index = i;
            break;
        }
    }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    EGLConfig match = configs[exact_match_index];

    static const GLint glesAtt[] =
    { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EGLContext c = eglCreateContext(d, match, EGL_NO_CONTEXT, glesAtt);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EGLSurface s = eglCreateWindowSurface(d, match, win, 0);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglMakeCurrent(d, s, s, c);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    for (int i = 0; i < 120; i++) {
    fprintf(stderr, "%s:%d arrive %d\n", __func__, __LINE__, i);
        glViewport(0, 0, 256, 256);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        glClearColor(1.0f, 0.0f, i / 120.0f, 1.0f);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        glClear(GL_COLOR_BUFFER_BIT);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        glFinish();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        eglSwapBuffers(d, s);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        t->loop();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EXPECT_EQ(EGL_TRUE, eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                       EGL_NO_CONTEXT));
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EXPECT_EQ(EGL_TRUE, eglDestroyContext(d, c));
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    EXPECT_EQ(EGL_TRUE, eglDestroySurface(d, s));
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    eglReleaseThread();
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    t.reset();

    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    android::opengl::getOpenGLObjectCounts(&afterTest);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    for (int i = 0; i < beforeTest.size(); i++) {
    fprintf(stderr, "%s:%d arrive %d\n", __func__, __LINE__, i);
        EXPECT_TRUE(beforeTest[i] >= afterTest[i]);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
}

}  // namespace aemu
