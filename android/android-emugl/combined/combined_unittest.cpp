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
#include "android/emulation/testing/TestVmLock.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"

#include <EGL/egl.h>

#include <stdio.h>

using android::AndroidPipe;
using android::base::System;
using android::HostGoldfishPipeDevice;

TEST(Combined, Hello) {
    android_initOpenglesEmulation();

    int maj;
    int min;

    android_startOpenglesRenderer(256, 256, 1, 28, &maj, &min);
    fprintf(stderr, "%s: maj %d min %d.\n", __func__, maj, min);

    char* vendor;
    char* renderer;
    char* version;
    android_getOpenglesHardwareStrings(&vendor, &renderer, &version);

    fprintf(stderr, "%s: GL strings; [%s] [%s] [%s].\n", __func__,
            vendor, renderer, version);

    android::TestVmLock* testVmLock = android::TestVmLock::getInstance();
    auto dev = HostGoldfishPipeDevice::get();

    android_init_opengles_pipe();

    auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    fprintf(stderr, "%s: disp: %p\n", __func__, disp);

    auto res = eglInitialize(disp, &maj, &min);

    fprintf(stderr, "%s: initialized. res: %d. maj min %d %d\n", __func__, res, maj, min);

    android_stopOpenglesRenderer(true);
}