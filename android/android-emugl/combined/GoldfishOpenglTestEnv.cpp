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

#include <cutils/properties.h>  // for property...
#include <stdint.h>             // for uint64_t
#include <stdio.h>              // for fprintf
#include <fstream>              // for ofstream
#include <functional>           // for __base
#include <string>               // for operator==

#include "render-utils/Renderer.h"                                        // for Renderer
#include "VulkanDispatch.h"                                  // for vkDispatch
#include "android/avd/info.h"                                // for avdInfo_...
#include "android/avd/util.h"                                // for AVD_PHONE

#include "aemu/base/files/MemStream.h"                    // for MemStream
#include "aemu/base/files/PathUtils.h"                    // for pj
#include "android/base/system/System.h"                      // for System
#include "android/base/testing/TestTempDir.h"                // for TestTempDir
#include "android/cmdline-option.h"                          // for AndroidO...
#include "android/console.h"                                 // for getConso...
#include "host-common/AndroidPipe.h"                   // for AndroidPipe
#include "host-common/HostmemIdMapping.h"              // for android_...
#include "host-common/address_space_device.hpp"        // for goldfish...
#include "host-common/address_space_graphics.h"        // for AddressS...
#include "host-common/address_space_graphics_types.h"  // for Consumer...
#include "android/emulation/control/AndroidAgentFactory.h"   // for initiali...
#include "android/emulation/control/callbacks.h"             // for LineCons...
#include "host-common/vm_operations.h"         // for Snapshot...
#include "android/emulation/hostdevices/HostAddressSpace.h"  // for HostAddr...
#include "host-common/HostGoldfishPipe.h"  // for HostGold...
#include "android/emulation/testing/TemporaryCommandLineOptions.h"
#include "host-common/FeatureControl.h"  // for setEnabl...
#include "host-common/Features.h"        // for GLAsyncSwap
#include "android/opengl/GLProcessPipe.h"
#include "host-common/opengl/emugl_config.h"  // for emuglCon...
#include "android/opengles-pipe.h"        // for android_...
#include "host-common/opengles.h"             // for android_...
#include "host-common/refcount-pipe.h"        // for android_...
#include "android/skin/winsys.h"          // for WINSYS_G...
#include "snapshot/interface.h"   // for androidS...

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
using android::base::PathUtils;
using android::base::pj;
using android::base::System;

using android::emulation::asg::AddressSpaceGraphicsContext;
using android::emulation::asg::ConsumerCallbacks;
using android::emulation::asg::ConsumerInterface;

static constexpr int kWindowSize = 256;

static GoldfishOpenglTestEnv* sTestEnv = nullptr;

static android::base::TestTempDir* sTestContentDir = nullptr;

// static
std::vector<const char*> GoldfishOpenglTestEnv::getTransportsToTest() {
    return {
            "pipe",
    };
}

AndroidOptions emptyOptions{};
AndroidOptions* sAndroid_cmdLineOptions = &emptyOptions;
AvdInfo* sAndroid_avdInfo = nullptr;
AndroidHwConfig s_hwConfig = {0};
AvdInfoParams sAndroid_avdInfoParams = {0};
std::string sCmdlLine;
LanguageSettings s_languageSettings = {0};
AUserConfig* s_userConfig = nullptr;
bool sKeyCodeForwarding = false;

// /* this indicates that guest has mounted data partition */
int s_guest_data_partition_mounted = 0;
// /* this indicates that guest has boot completed */
bool s_guest_boot_completed = 0;
bool s_arm_snapshot_save_completed = 0;
bool s_host_emulator_is_headless = 0;
// /* are we using the emulator in the android mode or plain qemu? */
bool s_android_qemu_mode = true;
// /* are we using android-emu libraries for a minimal configuration? */
// Min config mode and fuchsia mode are equivalent, at least for now.
bool s_min_config_qemu_mode = false;
// /* is android-emu running Fuchsia? */
int s_android_snapshot_update_timer = 0;

static const QAndroidGlobalVarsAgent gMockAndroidGlobalVarsAgent = {
        .avdParams = []() { return &sAndroid_avdInfoParams; },
        .avdInfo =
                []() {
                    // Do not access the info before it is injected!
                    assert(sAndroid_avdInfo != nullptr);
                    return sAndroid_avdInfo;
                },
        .hw = []() { return &s_hwConfig; },
        // /* this indicates that guest has mounted data partition */
        .guest_data_partition_mounted =
                []() { return s_guest_data_partition_mounted; },

        // /* this indicates that guest has boot completed */
        .guest_boot_completed = []() { return s_guest_boot_completed; },

        .arm_snapshot_save_completed =
                []() { return s_arm_snapshot_save_completed; },

        .host_emulator_is_headless =
                []() { return s_host_emulator_is_headless; },

        // /* are we using the emulator in the android mode or plain qemu? */
        .android_qemu_mode = []() { return s_android_qemu_mode; },

        // /* are we using android-emu libraries for a minimal configuration? */
        .min_config_qemu_mode = []() { return s_min_config_qemu_mode; },

        // /* is android-emu running Fuchsia? */
        .is_fuchsia = []() { return s_min_config_qemu_mode; },

        .android_snapshot_update_timer =
                []() { return s_android_snapshot_update_timer; },
        .language = []() { return &s_languageSettings; },
        .use_keycode_forwarding = []() { return sKeyCodeForwarding; },
        .userConfig = []() { return s_userConfig; },
        .android_cmdLineOptions = []() { return sAndroid_cmdLineOptions; },
        .inject_cmdLineOptions =
                [](AndroidOptions* opts) { sAndroid_cmdLineOptions = opts; },
        .has_cmdLineOptions =
                []() {
                    return gMockAndroidGlobalVarsAgent.android_cmdLineOptions() != nullptr;
                },
        .android_cmdLine = []() { return (const char*)sCmdlLine.c_str(); },
        .inject_android_cmdLine =
                [](const char* cmdline) { sCmdlLine = cmdline; },
        .inject_language =
                [](char* language, char* country, char* locale) {
                    s_languageSettings.language = language;
                    s_languageSettings.country = country;
                    s_languageSettings.locale = locale;
                    s_languageSettings.changing_language_country_locale =
                            language || country || locale;
                },
        .inject_userConfig = [](AUserConfig* config) { s_userConfig = config; },
        .set_keycode_forwarding =
                [](bool enabled) { sKeyCodeForwarding = enabled; },
        .inject_AvdInfo = [](AvdInfo* avd) { sAndroid_avdInfo = avd; },

        // /* this indicates that guest has mounted data partition */
        .set_guest_data_partition_mounted =
                [](int guest_data_partition_mounted) {
                    s_guest_data_partition_mounted =
                            guest_data_partition_mounted;
                },

        // /* this indicates that guest has boot completed */
        .set_guest_boot_completed =
                [](bool guest_boot_completed) {
                    s_guest_boot_completed = guest_boot_completed;
                },

        .set_arm_snapshot_save_completed =
                [](bool arm_snapshot_save_completed) {
                    s_arm_snapshot_save_completed = arm_snapshot_save_completed;
                },

        .set_host_emulator_is_headless =
                [](bool host_emulator_is_headless) {
                    s_host_emulator_is_headless = host_emulator_is_headless;
                },

        // /* are we using the emulator in the android mode or plain qemu? */
        .set_android_qemu_mode =
                [](bool android_qemu_mode) {
                    s_android_qemu_mode = android_qemu_mode;
                },

        // /* are we using android-emu libraries for a minimal configuration? */
        .set_min_config_qemu_mode =
                [](bool min_config_qemu_mode) {
                    s_min_config_qemu_mode = min_config_qemu_mode;
                },

        // /* is android-emu running Fuchsia? */
        .set_is_fuchsia = [](bool is_fuchsia) { s_min_config_qemu_mode = is_fuchsia; },

        .set_android_snapshot_update_timer =
                [](int android_snapshot_update_timer) {
                    s_android_snapshot_update_timer =
                            android_snapshot_update_timer;
                }

};


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

    const QAndroidGlobalVarsAgent* const android_get_QAndroidGlobalVarsAgent()
            const {
        return &gMockAndroidGlobalVarsAgent;
    }
};

GoldfishOpenglTestEnv::GoldfishOpenglTestEnv() {
    android::emulation::injectConsoleAgents(GolfishMockConsoleFactory());
    sTestEnv = this;
    sTestContentDir =
            new android::base::TestTempDir("goldfish_opengl_snapshot_test_dir");

    getConsoleAgents()->settings->inject_AvdInfo(
            avdInfo_newCustom("goldfish_opengl_test", 28, "x86_64", "x86_64",
                              true /* is google APIs */, AVD_PHONE, nullptr));

    avdInfo_setCustomContentPath(getConsoleAgents()->settings->avdInfo(),
                                 sTestContentDir->path());
    auto customHwIniPath = pj(sTestContentDir->path(), "hardware.ini");

    std::ofstream hwIniPathTouch(
            PathUtils::asUnicodePath(customHwIniPath.data()).c_str(), std::ios::out);
    hwIniPathTouch << "test ini";
    hwIniPathTouch.close();

    avdInfo_setCustomCoreHwIniPath(getConsoleAgents()->settings->avdInfo(),
                                   customHwIniPath.c_str());

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
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanQueueSubmitWithCommands, false);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanBatchedDescriptorSetUpdate, false);

    bool useHostGpu =
            System::get()->envGet("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    emugl::vkDispatch(!useHostGpu /* use test ICD if not with host gpu */);

    getConsoleAgents()->settings->hw()->hw_gltransport_asg_writeBufferSize =
            262144;
    getConsoleAgents()->settings->hw()->hw_gltransport_asg_writeStepSize = 8192;
    getConsoleAgents()->settings->hw()->hw_gltransport_asg_dataRingSize =
            131072;
    getConsoleAgents()->settings->hw()->hw_gltransport_drawFlushInterval = 800;

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
    mTestEnvCmdLineOptions.read_only = 0;
    mTestEnvCmdLineOptions.no_snapshot = 0;
    mTestEnvCmdLineOptions.no_snapshot_save = 0;
    mTestEnvCmdLineOptions.no_snapshot_load = 0;

    auto openglesRenderer = android_getOpenglesRenderer().get();

    ConsumerInterface interface = {
            // create
            [openglesRenderer](struct asg_context context,
                               android::base::Stream* loadStream,
                               ConsumerCallbacks callbacks,
                               uint32_t virtioGpuContextId, uint32_t virtioGpuCapsetId,
                               std::optional<std::string> nameOpt) {
                return openglesRenderer->addressSpaceGraphicsConsumerCreate(
                        context, loadStream, callbacks, virtioGpuContextId, virtioGpuCapsetId,
                        nameOpt);
            },
            // destroy
            [openglesRenderer](void* consumer) {
                return openglesRenderer->addressSpaceGraphicsConsumerDestroy(
                        consumer);
            },
            // pre save
            [openglesRenderer](void* consumer) {
                return openglesRenderer->addressSpaceGraphicsConsumerPreSave(
                        consumer);
            },
            // global presave
            [openglesRenderer]() {
                return openglesRenderer->pauseAllPreSave();
            },
            // save
            [openglesRenderer](void* consumer, android::base::Stream* stream) {
                return openglesRenderer->addressSpaceGraphicsConsumerSave(
                        consumer, stream);
            },
            // global postsave
            [openglesRenderer]() { return openglesRenderer->resumeAll(); },
            // postSave
            [openglesRenderer](void* consumer) {
                return openglesRenderer->addressSpaceGraphicsConsumerPostSave(
                        consumer);
            },
            // postLoad
            [openglesRenderer](void* consumer) {
                openglesRenderer
                        ->addressSpaceGraphicsConsumerRegisterPostLoadRenderThread(
                                consumer);
            },
            // global preload
            [openglesRenderer]() {
                android::opengl::forEachProcessPipeIdRunAndErase(
                        [openglesRenderer](uint64_t id) {
                            openglesRenderer->cleanupProcGLObjects(id);
                        });
                openglesRenderer->waitForProcessCleanup();
            },
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
    android_waitForOpenglesProcessCleanup();
}

void GoldfishOpenglTestEnv::saveSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->saveSnapshot(stream);
    HostGoldfishPipeDevice::get()->saveSnapshot(stream);
}

void GoldfishOpenglTestEnv::loadSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->loadSnapshot(stream);
    HostGoldfishPipeDevice::get()->loadSnapshot(stream);
}
