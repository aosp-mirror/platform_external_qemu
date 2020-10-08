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

#include <cutils/properties.h>                               // for property...
#include <stdint.h>                                          // for uint64_t
#include <stdio.h>                                           // for fprintf
#include <fstream>                                           // for ofstream
#include <functional>                                        // for __base
#include <string>                                            // for operator==

#include "Renderer.h"                                        // for Renderer
#include "VulkanDispatch.h"                                  // for vkDispatch
#include "android/avd/info.h"                                // for avdInfo_...
#include "android/avd/util.h"                                // for AVD_PHONE
#include "android/base/StringView.h"                         // for StringView
#include "android/base/files/MemStream.h"                    // for MemStream
#include "android/base/files/PathUtils.h"                    // for pj
#include "android/base/system/System.h"                      // for System
#include "android/base/testing/TestTempDir.h"                // for TestTempDir
#include "android/cmdline-option.h"                          // for AndroidO...
#include "android/console.h"                                 // for getConso...
#include "android/emulation/AndroidPipe.h"                   // for AndroidPipe
#include "android/emulation/HostmemIdMapping.h"              // for android_...
#include "android/emulation/address_space_device.hpp"        // for goldfish...
#include "android/emulation/address_space_graphics.h"        // for AddressS...
#include "android/emulation/address_space_graphics_types.h"  // for Consumer...
#include "android/emulation/control/AndroidAgentFactory.h"   // for initiali...
#include "android/emulation/control/callbacks.h"             // for LineCons...
#include "android/emulation/control/vm_operations.h"         // for Snapshot...
#include "android/emulation/hostdevices/HostAddressSpace.h"  // for HostAddr...
#include "android/emulation/hostdevices/HostGoldfishPipe.h"  // for HostGold...
#include "android/featurecontrol/FeatureControl.h"           // for setEnabl...
#include "android/featurecontrol/Features.h"                 // for GLAsyncSwap
#include "android/globals.h"                                 // for android_hw
#include "android/opengl/emugl_config.h"                     // for emuglCon...
#include "android/opengles-pipe.h"                           // for android_...
#include "android/opengles.h"                                // for android_...
#include "android/refcount-pipe.h"                           // for android_...
#include "android/skin/winsys.h"                             // for WINSYS_G...
#include "android/snapshot/interface.h"                      // for androidS...

namespace aemu {
class AndroidBufferQueue;
class AndroidWindow;
class AndroidWindowBuffer;
class ClientComposer;
class Display;
}  // namespace aemu

using aemu::AndroidBufferQueue;
using aemu::AndroidWindow;
using aemu::AndroidWindowBuffer;
using aemu::ClientComposer;
using aemu::Display;

using android::AndroidPipe;
using android::HostAddressSpaceDevice;
using android::HostGoldfishPipeDevice;
using android::base::pj;
using android::base::System;

using android::emulation::asg::AddressSpaceGraphicsContext;
using android::emulation::asg::ConsumerCallbacks;
using android::emulation::asg::ConsumerInterface;

static constexpr int kWindowSize = 256;

static GoldfishOpenglTestEnv* sTestEnv = nullptr;

static android::base::TestTempDir* sTestContentDir = nullptr;

static AndroidOptions sTestEnvCmdLineOptions;

// static
std::vector<const char*> GoldfishOpenglTestEnv::getTransportsToTest() {
    return {
            "pipe",
    };
}

static const SnapshotCallbacks* sSnapshotCallbacks = nullptr;

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm stop\n");
            return true;
        },
        .vmStart = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm start\n");
            return true;
        },
        .vmReset =
                []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
        .vmShutdown =
                []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
        .vmPause = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm pause\n");
            return true;
        },
        .vmResume = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm resume\n");
            return true;
        },
        .vmIsRunning = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm is running\n");
            return true;
        },
        .snapshotList =
                [](void*, LineConsumerCallback, LineConsumerCallback) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot list\n");
            return true;
        },
        .snapshotSave = [](const char* name,
                           void* opaque,
                           LineConsumerCallback) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot save\n");
            sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onStart(opaque, name);
            auto testEnv = GoldfishOpenglTestEnv::get();
            if (testEnv->snapshotStream)
                delete testEnv->snapshotStream;
            testEnv->snapshotStream = new android::base::MemStream;
            testEnv->saveSnapshot(testEnv->snapshotStream);
            sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onEnd(opaque, name, 0);
            return true;
        },
        .snapshotLoad = [](const char* name,
                           void* opaque,
                           LineConsumerCallback) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot load\n");
            sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onStart(opaque, name);
            auto testEnv = GoldfishOpenglTestEnv::get();
            testEnv->loadSnapshot(testEnv->snapshotStream);
            sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onEnd(opaque, name, 0);
            testEnv->snapshotStream->rewind();
            return true;
        },
        .snapshotDelete = [](const char* name,
                             void* opaque,
                             LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot delete\n");
            return true;
        },
        .snapshotExport = [](const char* snapshot,
                             const char* dest,
                             void* opaque,
                             LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot export image\n");
            return true;
        },
        .snapshotRemap = [](bool shared,
                            void* opaque,
                            LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot remap\n");
            return true;
        },
        .setSnapshotCallbacks =
                [](void* opaque, const SnapshotCallbacks* callbacks) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set snapshot callbacks\n");
                    sSnapshotCallbacks = callbacks;
                },
        .mapUserBackedRam =
                [](uint64_t gpa, void* hva, uint64_t size) {
                    (void)size;
                    HostAddressSpaceDevice::get()->setHostAddrByPhysAddr(gpa,
                                                                         hva);
                },
        .unmapUserBackedRam =
                [](uint64_t gpa, uint64_t size) {
                    (void)size;
                    HostAddressSpaceDevice::get()->unsetHostAddrByPhysAddr(gpa);
                },
        .getVmConfiguration =
                [](VmConfiguration* out) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: get vm configuration\n");
                },
        .setFailureReason =
                [](const char* name, int failureReason) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set failure reason\n");
                },
        .setExiting =
                []() {
                    fprintf(stderr, "goldfish-opengl vm ops: set exiting\n");
                },
        .allowRealAudio =
                [](bool allow) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: allow real audio\n");
                },
        .physicalMemoryGetAddr =
                [](uint64_t gpa) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: physical memory get "
                            "addr\n");
                    void* res = HostAddressSpaceDevice::get()->getHostAddr(gpa);
                    if (!res)
                        return (void*)(uintptr_t)gpa;
                    return res;
                },
        .isRealAudioAllowed =
                [](void) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: is real audiop allowed\n");
                    return true;
                },
        .setSkipSnapshotSave =
                [](bool used) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set skip snapshot save\n");
                },
        .isSnapshotSaveSkipped =
                []() {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: is snapshot save "
                            "skipped\n");
                    return false;
                },
        .hostmemRegister = android_emulation_hostmem_register,
        .hostmemUnregister = android_emulation_hostmem_unregister,
        .hostmemGetInfo = android_emulation_hostmem_get_info,
};

class GolfishMockConsoleFactory
    : public android::emulation::AndroidConsoleFactory {
    const QAndroidVmOperations* const android_get_QAndroidVmOperations()
            const override {
        return &sQAndroidVmOperations;
    }
};

GoldfishOpenglTestEnv::GoldfishOpenglTestEnv() {
    android::emulation::injectConsoleAgents(GolfishMockConsoleFactory());
    sTestEnv = this;
    sTestContentDir =
            new android::base::TestTempDir("goldfish_opengl_snapshot_test_dir");

    android_avdInfo =
            avdInfo_newCustom("goldfish_opengl_test", 28, "x86_64", "x86_64",
                              true /* is google APIs */, AVD_PHONE);

    avdInfo_setCustomContentPath(android_avdInfo, sTestContentDir->path());
    auto customHwIniPath = pj(sTestContentDir->path(), "hardware.ini");

    std::ofstream hwIniPathTouch(customHwIniPath, std::ios::out);
    hwIniPathTouch << "test ini";
    hwIniPathTouch.close();

    avdInfo_setCustomCoreHwIniPath(android_avdInfo, customHwIniPath.c_str());

    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR",
                          System::get()->getProgramDirectory());

    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLESDynamicVersion, true);
    android::featurecontrol::setEnabledOverride(android::featurecontrol::GLDMA,
                                                false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLAsyncSwap, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::RefCountPipe, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GLDirectMem, true);
    android::featurecontrol::setEnabledOverride(android::featurecontrol::Vulkan,
                                                true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanSnapshots, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanNullOptionalStrings, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanIgnoredHandles, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VirtioGpuNext, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VirtioGpuNativeSync, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanShaderFloat16Int8, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::GuestUsesAngle, false);

    bool useHostGpu =
            System::get()->envGet("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    emugl::vkDispatch(!useHostGpu /* use test ICD if not with host gpu */);

    android_hw->hw_gltransport_asg_writeBufferSize = 262144;
    android_hw->hw_gltransport_asg_writeStepSize = 8192;
    android_hw->hw_gltransport_asg_dataRingSize = 131072;
    android_hw->hw_gltransport_drawFlushInterval = 800;

    EmuglConfig config;

    emuglConfig_init(
            &config, true /* gpu enabled */, "auto",
            useHostGpu ? "host" : "swiftshader_indirect", /* gpu mode, option */
            64,                                           /* bitness */
            true,                                         /* no window */
            false,                                        /* blacklisted */
            false, /* has guest renderer */
            WINSYS_GLESBACKEND_PREFERENCE_AUTO, false);

    emuglConfig_setupEnv(&config);

    // Set fake guest properties
    property_set("ro.kernel.qemu.gles", "1");
    property_set("qemu.sf.lcd_density", "420");
    property_set("ro.kernel.qemu.gltransport", "asg");
    property_set("ro.kernel.qemu.gltransport.drawFlushInterval", "800");

    android_initOpenglesEmulation();

    int maj;
    int min;

    android_startOpenglesRenderer(
            kWindowSize, kWindowSize, 1, 28, getConsoleAgents()->vm,
            getConsoleAgents()->emu, getConsoleAgents()->multi_display, &maj,
            &min);

    androidSnapshot_initialize(getConsoleAgents()->vm, getConsoleAgents()->emu);

    androidSnapshot_setDiskSpaceCheck(
            false /* do not check disk space > 2 GB */);
    androidSnapshot_quickbootSetShortRunCheck(
            false /* do not check if running < 1500 ms */);

    char* vendor = nullptr;
    char* renderer = nullptr;
    char* version = nullptr;

    android_getOpenglesHardwareStrings(&vendor, &renderer, &version);

    printf("%s: GL strings; [%s] [%s] [%s].\n", __func__, vendor, renderer,
           version);

    android::emulation::goldfish_address_space_set_vm_operations(
            getConsoleAgents()->vm);

    HostAddressSpaceDevice::get();
    HostGoldfishPipeDevice::get();

    android_init_opengles_pipe();

    android_init_refcount_pipe();

    // Command line options that enable snapshots
    sTestEnvCmdLineOptions.read_only = 0;
    sTestEnvCmdLineOptions.no_snapshot = 0;
    sTestEnvCmdLineOptions.no_snapshot_save = 0;
    sTestEnvCmdLineOptions.no_snapshot_load = 0;

    android_cmdLineOptions = &sTestEnvCmdLineOptions;

    auto openglesRenderer = android_getOpenglesRenderer().get();

    ConsumerInterface interface = {
            // create
            [openglesRenderer](struct asg_context context,
                               ConsumerCallbacks callbacks) {
                return openglesRenderer->addressSpaceGraphicsConsumerCreate(
                        context, callbacks);
            },
            // destroy
            [openglesRenderer](void* consumer) {
                return openglesRenderer->addressSpaceGraphicsConsumerDestroy(
                        consumer);
            },
            // TODO
            // save
            [](void* consumer, android::base::Stream* stream) {},
            // load
            [](void* consumer, android::base::Stream* stream) {},
    };
    AddressSpaceGraphicsContext::setConsumer(interface);
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

void GoldfishOpenglTestEnv::clear() {
    HostGoldfishPipeDevice::get()->clear();
    HostAddressSpaceDevice::get()->clear();
}

void GoldfishOpenglTestEnv::saveSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->saveSnapshot(stream);
    HostGoldfishPipeDevice::get()->saveSnapshot(stream);
}

void GoldfishOpenglTestEnv::loadSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->loadSnapshot(stream);
    HostGoldfishPipeDevice::get()->loadSnapshot(stream);
}
