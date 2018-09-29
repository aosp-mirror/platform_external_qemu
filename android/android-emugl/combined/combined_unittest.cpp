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
#include "android/base/system/System.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/emugl_config.h"

#include "GrallocDispatch.h"

#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <string>

#include <stdio.h>

using android::AndroidPipe;
using android::base::pj;
using android::base::System;
using android::HostGoldfishPipeDevice;

// Runs the goldfish-opengl guest driver with a host-side goldfish-pipe
// and ashmem device. Currently tests the following:
// - Host-side initialization: configAndStartRenderer pattern
// - Establishing a pipe connection
// - Creating gralloc color buffers
// - EGL initialization
// - GLES context creation and state query
// Known unsupported:
// - eglSwapBuffers (need ANativeWindow, buffer queue)
TEST(Combined, Hello) {
    EmuglConfig config;

    emuglConfig_init(
            &config,
            true /* gpu enabled */,
            "auto", "swiftshader_indirect", /* gpu mode, option */
            64, /* bitness */
            true, /* no window */
            false, /* blacklisted */
            false, /* has guest renderer */
            WINSYS_GLESBACKEND_PREFERENCE_AUTO);

    emuglConfig_setupEnv(&config);

    android_initOpenglesEmulation();

    int maj;
    int min;

    android_startOpenglesRenderer(256, 256, 1, 28, &maj, &min);

    char* vendor = nullptr;
    char* renderer = nullptr;
    char* version = nullptr;

    android_getOpenglesHardwareStrings(&vendor, &renderer, &version);

    printf(
        "%s: GL strings; [%s] [%s] [%s].\n", __func__,
        vendor, renderer, version);

    HostGoldfishPipeDevice::get();

    android_init_opengles_pipe();

    auto grallocPath =
        pj(System::get()->getProgramDirectory(),
           "lib64",
           "gralloc.ranchu" LIBSUFFIX);

    struct framebuffer_device_t* fb0Dev;
    struct alloc_device_t* gpu0Dev;
    struct gralloc_module_t* fb0Module;
    struct gralloc_module_t* gpu0Module;
    load_gralloc_module(
        grallocPath.c_str(),
        &fb0Dev,
        &gpu0Dev,
        &fb0Module,
        &gpu0Module);

    EXPECT_NE(nullptr, fb0Dev);
    EXPECT_NE(nullptr, gpu0Dev);
    EXPECT_NE(nullptr, fb0Module);
    EXPECT_NE(nullptr, gpu0Module);

    buffer_handle_t buffer;
    int stride;
    gpu0Dev->alloc(gpu0Dev, 256, 256, HAL_PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_HW_RENDER, &buffer, &stride);
    gpu0Module->registerBuffer(gpu0Module, buffer);

    auto dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    auto res = eglInitialize(dpy, &maj, &min);

    EXPECT_EQ(EGL_TRUE, res);

    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT, EGL_NONE };

    EGLConfig eglConfig;
    int numConfigs;

    EXPECT_EQ(EGL_TRUE, eglChooseConfig(dpy, configAttribs, &eglConfig, 1, &numConfigs))
        << "Could not find GLES2 config!";

    EXPECT_GT(numConfigs, 0);

    static const EGLint pbufAttribs[] =
        { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };

    EGLSurface surface = eglCreatePbufferSurface(dpy, eglConfig, pbufAttribs);

    EXPECT_NE(EGL_NO_SURFACE, surface)
        << "Could not create pbuffer surface!";

    static const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE,
    };

    EGLContext context =
        eglCreateContext(
            dpy, eglConfig, EGL_NO_CONTEXT, contextAttribs);

    EXPECT_NE(EGL_NO_CONTEXT, context)
        << "Could not create context!";

    EXPECT_EQ(EGL_TRUE, eglMakeCurrent(dpy, surface, surface, context))
        << "eglMakeCurrent failed!";

    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);

    EXPECT_EQ(GL_NO_ERROR, glGetError())
        << "GL error when querying viewport!";

    EXPECT_EQ(0, viewport[0]);
    EXPECT_EQ(0, viewport[1]);
    EXPECT_EQ(1, viewport[2]);
    EXPECT_EQ(1, viewport[3]);

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglReleaseThread();

    eglTerminate(dpy);

    gpu0Module->unregisterBuffer(gpu0Module, buffer);
    gpu0Module->unregisterBuffer(gpu0Module, buffer);

    android_stopOpenglesRenderer(true);
}