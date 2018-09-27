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

#include "android/base/system/System.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/emugl_config.h"

#include <EGL/egl.h>

#include <string>

#include <stdio.h>

using android::AndroidPipe;
using android::base::System;
using android::HostGoldfishPipeDevice;

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

    auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    auto res = eglInitialize(disp, &maj, &min);

    EXPECT_EQ(EGL_TRUE, res);

    eglTerminate(disp);

    android_stopOpenglesRenderer(true);
}
