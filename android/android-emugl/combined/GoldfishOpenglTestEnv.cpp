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
#include "GoldfishOpenglTestEnv.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/opengl/emugl_config.h"
#include "android/opengles-pipe.h"
#include "android/opengles.h"
#include "android/refcount-pipe.h"

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"
#include "AndroidWindowBuffer.h"
#include "ClientComposer.h"
#include "Display.h"
#include "GrallocDispatch.h"
#include "VulkanDispatch.h"

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
using android::base::pj;
using android::base::System;
using android::HostGoldfishPipeDevice;

static constexpr int kWindowSize = 256;

GoldfishOpenglTestEnv::GoldfishOpenglTestEnv() {
    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR",
                          System::get()->getProgramDirectory());

    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLESDynamicVersion, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDMA, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLAsyncSwap, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::RefCountPipe, true);

    emugl::vkDispatch(true /* get the test ICD */);

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

    android_init_refcount_pipe();
}

GoldfishOpenglTestEnv::~GoldfishOpenglTestEnv() {
    AndroidPipe::Service::resetAll();
    android_stopOpenglesRenderer(true);
}