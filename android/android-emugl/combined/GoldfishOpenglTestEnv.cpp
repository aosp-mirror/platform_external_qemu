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

#include "android/avd/info.h"
#include "android/base/files/MemStream.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengles-pipe.h"
#include "android/opengles.h"
#include "android/refcount-pipe.h"
#include "android/snapshot/interface.h"

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

#include <fstream>
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

extern const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent;

extern const QAndroidVmOperations* const gQAndroidVmOperations;

static GoldfishOpenglTestEnv* sTestEnv = nullptr;

static android::base::TestTempDir* sTestContentDir = nullptr;

GoldfishOpenglTestEnv::GoldfishOpenglTestEnv() {
    sTestEnv = this;
    sTestContentDir = new android::base::TestTempDir("goldfish_opengl_snapshot_test_dir");

    android_avdInfo = avdInfo_newCustom(
        "goldfish_opengl_test",
        28,
        "x86_64",
        "x86_64",
        true /* is google APIs */,
        AVD_PHONE);

    avdInfo_setCustomContentPath(android_avdInfo, sTestContentDir->path());
    auto customHwIniPath =
        pj(sTestContentDir->path(), "hardware.ini");

    std::ofstream hwIniPathTouch(customHwIniPath, std::ios::out);
    hwIniPathTouch << "test ini";
    hwIniPathTouch.close();


    avdInfo_setCustomCoreHwIniPath(android_avdInfo, customHwIniPath.c_str());

    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR",
                          System::get()->getProgramDirectory());

    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLESDynamicVersion, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDMA, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLAsyncSwap, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::RefCountPipe, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDirectMem, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::Vulkan, true);

    bool useHostGpu =
            System::get()->envGet("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    emugl::vkDispatch(!useHostGpu /* use test ICD if not with host gpu */);

    EmuglConfig config;

    emuglConfig_init(&config, true /* gpu enabled */, "auto",
                     useHostGpu ? "host" : "swiftshader_indirect", /* gpu mode, option */
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
        kWindowSize, kWindowSize, 1, 28, gQAndroidVmOperations,
        gQAndroidEmulatorWindowAgent, &maj, &min);

    androidSnapshot_initialize(
        gQAndroidVmOperations,
        gQAndroidEmulatorWindowAgent);

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
    sTestEnv = nullptr;
}

// static
GoldfishOpenglTestEnv* GoldfishOpenglTestEnv::get() {
    return sTestEnv;
}

void GoldfishOpenglTestEnv::saveSnapshot(android::base::Stream* stream) {
    HostGoldfishPipeDevice::get()->saveSnapshot(stream);
}

void GoldfishOpenglTestEnv::loadSnapshot(android::base::Stream* stream) {
    HostGoldfishPipeDevice::get()->loadSnapshot(stream);
}

static const SnapshotCallbacks* sSnapshotCallbacks = nullptr;

static const QAndroidVmOperations sQAndroidVmOperations = {
    .vmStop = []() -> bool { fprintf(stderr, "goldfish-opengl vm ops: vm stop\n"); return true; },
    .vmStart = []() -> bool { fprintf(stderr, "goldfish-opengl vm ops: vm start\n"); return true; },
    .vmReset = []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
    .vmShutdown = []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
    .vmPause = []() -> bool { fprintf(stderr, "goldfish-opengl vm ops: vm pause\n"); return true; },
    .vmResume = []() -> bool { fprintf(stderr, "goldfish-opengl vm ops: vm resume\n"); return true; },
    .vmIsRunning = []() -> bool { fprintf(stderr, "goldfish-opengl vm ops: vm is running\n"); return true; },
    .snapshotList = [](void*, LineConsumerCallback, LineConsumerCallback) -> bool { fprintf(stderr, "goldfish-opengl vm ops: snapshot list\n"); return true; },
    .snapshotSave = [](const char* name, void* opaque, LineConsumerCallback) -> bool {
        fprintf(stderr, "goldfish-opengl vm ops: snapshot save\n");
        sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onStart(opaque, name);
        auto testEnv = GoldfishOpenglTestEnv::get();
        if (testEnv->snapshotStream) delete testEnv->snapshotStream;
        testEnv->snapshotStream = new android::base::MemStream;
        testEnv->saveSnapshot(testEnv->snapshotStream);
        sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onEnd(opaque, name, 0);
        return true;
    },
    .snapshotLoad = [](const char* name, void* opaque, LineConsumerCallback) -> bool {
        fprintf(stderr, "goldfish-opengl vm ops: snapshot load\n");
        sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onStart(opaque, name);
        auto testEnv = GoldfishOpenglTestEnv::get();
        testEnv->loadSnapshot(testEnv->snapshotStream);
        sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onEnd(opaque, name, 0);
        return true;
    },
    .snapshotDelete = [](const char* name, void* opaque, LineConsumerCallback errConsumer) -> bool {
        fprintf(stderr, "goldfish-opengl vm ops: snapshot delete\n");
        return true;
    },
    .snapshotRemap = [](bool shared, void* opaque, LineConsumerCallback errConsumer) -> bool {
        fprintf(stderr, "goldfish-opengl vm ops: snapshot remap\n");
        return true;
    },
    .setSnapshotCallbacks = [](void* opaque, const SnapshotCallbacks* callbacks) {
        fprintf(stderr, "goldfish-opengl vm ops: set snapshot callbacks\n");
        sSnapshotCallbacks = callbacks;
    },
    .mapUserBackedRam = [](uint64_t gpa, void* hva, uint64_t size) {},
    .unmapUserBackedRam = [](uint64_t gpa, uint64_t size) {},
    .getVmConfiguration = [](VmConfiguration* out) {
        fprintf(stderr, "goldfish-opengl vm ops: get vm configuration\n");
     },
    .setFailureReason = [](const char* name, int failureReason) {
        fprintf(stderr, "goldfish-opengl vm ops: set failure reason\n");
     },
    .setExiting = []() {
        fprintf(stderr, "goldfish-opengl vm ops: set exiting\n");
     },
    .allowRealAudio = [](bool allow) {
        fprintf(stderr, "goldfish-opengl vm ops: allow real audio\n");
     },
    .physicalMemoryGetAddr = [](uint64_t gpa) {
        fprintf(stderr, "goldfish-opengl vm ops: physical memory get addr\n");
        return (void*)(uintptr_t)gpa;
     },
    .isRealAudioAllowed = [](void) {
        fprintf(stderr, "goldfish-opengl vm ops: is real audiop allowed\n");
        return true;
    },
    .setSkipSnapshotSave = [](bool used) {
        fprintf(stderr, "goldfish-opengl vm ops: set skip snapshot save\n");
    },
    .isSnapshotSaveSkipped = []() {
        fprintf(stderr, "goldfish-opengl vm ops: is snapshot save skipped\n");
        return false;
    },
};

const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;
