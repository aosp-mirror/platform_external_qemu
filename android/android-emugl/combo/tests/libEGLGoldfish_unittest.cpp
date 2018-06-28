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

#include "GoldfishOpenGLDispatch.h"
#include "GrallocDispatch.h"
#include "VirtualDisplay.h"

#include "android/base/system/System.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <vector>

using android::base::System;

namespace emugl {

TEST(libEGLGoldfish, Basic) {

    aemu::initVirtualDisplay();
    ANativeWindow* w = aemu::createWindow();

    auto& egl = loadGoldfishOpenGL()->egl;
    auto& gl = loadGoldfishOpenGL()->gles2;

    EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min;
    egl.eglInitialize(d, &maj, &min);

    egl.eglBindAPI(EGL_OPENGL_ES_API);
    EGLint total_num_configs = 0;
    egl.eglGetConfigs(d, 0, 0, &total_num_configs);

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
    egl.eglChooseConfig(d, configAttribs, configs.data(), total_num_configs, &total_compatible);

    EGLint exact_match_index = -1;
    for (EGLint i = 0; i < total_compatible; i++) {
        EGLint r, g, b;
        EGLConfig c = configs[i];
        egl.eglGetConfigAttrib(d, c, EGL_RED_SIZE, &r);
        egl.eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, &g);
        egl.eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, &b);

        if (r == wantedRedSize && g == wantedGreenSize && b == wantedBlueSize) {
            exact_match_index = i;
            break;
        }
    }

    EGLConfig match = configs[exact_match_index];

    static const GLint glesAtt[] =
    { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    EGLContext c = egl.eglCreateContext(d, match, EGL_NO_CONTEXT, glesAtt);

    EGLSurface s = egl.eglCreateWindowSurface(d, match, w, 0);

    egl.eglMakeCurrent(d, s, s, c);

    for (int i = 0; i < 120; i++) {
        gl.glViewport(0, 0, 256, 256);
        gl.glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        gl.glClear(GL_COLOR_BUFFER_BIT);
        gl.glFinish();
        egl.eglSwapBuffers(d, s);
        goldfishOnPost();
    }
}

} // namespace emugl

