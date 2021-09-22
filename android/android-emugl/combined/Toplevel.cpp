// Copyright 2018 The Android Open Source Project
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
#include "Toplevel.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/OnDemand.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/control/AndroidAgentFactory.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/emulation/MultiDisplay.h"
#include "android/emulation/HostmemIdMapping.h"
#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/hostdevices/HostGoldfishPipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/console.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengles-pipe.h"
#include "android/opengles.h"
#include "android/refcount-pipe.h"

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"
#include "AndroidWindowBuffer.h"
#include "ClientComposer.h"
#include "GrallocDispatch.h"
#include "Display.h"
#include "SurfaceFlinger.h"
#include "Vsync.h"
#include "VulkanDispatch.h"

#include <cutils/properties.h>
#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <map>
#include <string>
#include <unordered_set>

#include <stdio.h>

using aemu::AndroidBufferQueue;
using aemu::AndroidWindow;
using aemu::AndroidWindowBuffer;
using aemu::Display;
using aemu::SurfaceFlinger;
using aemu::Vsync;

using android::AndroidPipe;
using android::base::AutoLock;
using android::base::Lock;
using android::base::makeOnDemand;
using android::base::OnDemandT;
using android::base::pj;
using android::base::System;
using android::HostGoldfishPipeDevice;
using android::HostAddressSpaceDevice;

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
            return true;
        },
        .snapshotLoad = [](const char* name,
                           void* opaque,
                           LineConsumerCallback) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot load\n");
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

std::map<uint32_t, android::MultiDisplayInfo> mMultiDisplay;
static int32_t sDisplayX = 0;
static int32_t sDisplayY = 0;
static int32_t sDisplayW = 0;
static int32_t sDisplayH = 0;
static int32_t sDisplayDpi = 0;

static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .notifyDisplayChanges = []() {
            return true;
        },
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) -> int {
            fprintf(stderr,
                    "setMultiDisplay (mock): %d, x: %d, y: %d, w: %d, h: %d, "
                    "dpi: %d, flag: %d\n",
                    id, x, y, w, h, dpi, flag);
            mMultiDisplay[id] =
                    android::MultiDisplayInfo(x, y, w, h, dpi, flag, add);
        },
        .getMultiDisplay = [](uint32_t id,
                              int32_t* x,
                              int32_t* y,
                              uint32_t* w,
                              uint32_t* h,
                              uint32_t* dpi,
                              uint32_t* flag,
                              bool* enable) -> bool {
            fprintf(stderr, "getMultiDisplay (mock) id %u\n", id);
            if (x)
                *x = mMultiDisplay[id].pos_x;
            if (y)
                *y = mMultiDisplay[id].pos_y;
            if (w)
                *w = mMultiDisplay[id].width;
            if (h)
                *h = mMultiDisplay[id].height;
            if (dpi)
                *dpi = mMultiDisplay[id].dpi;

            return true;
        },
        .getNextMultiDisplay = [](int32_t start_id,
                                  uint32_t* id,
                                  int32_t* x,
                                  int32_t* y,
                                  uint32_t* w,
                                  uint32_t* h,
                                  uint32_t* dpi,
                                  uint32_t* flag,
                                  uint32_t* cb) -> bool {
            fprintf(stderr, "getNextMultiDisplay (mock) start_id %u\n",
                    start_id);
            uint32_t key;
            std::map<uint32_t, android::MultiDisplayInfo>::iterator i;
            if (start_id < 0) {
                key = 0;
            } else {
                key = start_id + 1;
            }
            i = mMultiDisplay.lower_bound(key);
            if (i == mMultiDisplay.end()) {
                return false;
            } else {
                if (id) {
                    *id = i->first;
                }
                if (x) {
                    *x = i->second.pos_x;
                }
                if (y) {
                    *y = i->second.pos_y;
                }
                if (w) {
                    *w = i->second.width;
                }
                if (h) {
                    *h = i->second.height;
                }
                if (dpi) {
                    *dpi = i->second.dpi;
                }
                if (flag) {
                    *flag = i->second.flag;
                }
                if (cb) {
                    *cb = i->second.cb;
                }
                return true;
            }
        },
        .isMultiDisplayEnabled = [](void) -> bool {
            return mMultiDisplay.size() > 1;
        },
        .getCombinedDisplaySize = [](uint32_t* width, uint32_t* height) {},
        .multiDisplayParamValidate = [](uint32_t id,
                                        uint32_t w,
                                        uint32_t h,
                                        uint32_t dpi,
                                        uint32_t flag) -> bool { return true; },
        .translateCoordination =
                [](uint32_t* x, uint32_t* y, uint32_t* displayId) -> bool {
            return true;
        },
        .setGpuMode = [](bool isGuestMode, uint32_t w, uint32_t h) {},
        .createDisplay = [](uint32_t* displayId) -> int {
            mMultiDisplay.emplace(*displayId, android::MultiDisplayInfo());
            return 0;
        },
        .destroyDisplay = [](uint32_t displayId) -> int {
            fprintf(stderr, "destroyDisplay (mock): %d\n", displayId);
            mMultiDisplay.erase(displayId);
            return 0;
        },
        .setDisplayPose = [](uint32_t displayId,
                             int32_t x,
                             int32_t y,
                             uint32_t w,
                             uint32_t h,
                             uint32_t dpi) -> int {
            mMultiDisplay[displayId].pos_x = x;
            mMultiDisplay[displayId].pos_y = y;
            mMultiDisplay[displayId].width = w;
            mMultiDisplay[displayId].height = h;
            mMultiDisplay[displayId].dpi = dpi;
            return 0;
        },
        .getDisplayPose = [](uint32_t displayId,
                             int32_t* x,
                             int32_t* y,
                             uint32_t* w,
                             uint32_t* h) -> int {
            if (x)
                *x = mMultiDisplay[displayId].pos_x;
            if (y)
                *y = mMultiDisplay[displayId].pos_y;
            if (w)
                *w = mMultiDisplay[displayId].width;
            if (h)
                *h = mMultiDisplay[displayId].height;
            return 0;
        },
        .getDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t* colorBuffer) -> int {
            *colorBuffer = mMultiDisplay[displayId].cb;
            return 0;
        },
        .getColorBufferDisplay = [](uint32_t colorBuffer,
                                    uint32_t* displayId) -> int {
            for (const auto& iter : mMultiDisplay) {
                if (iter.second.cb == colorBuffer) {
                    *displayId = iter.first;
                    return 0;
                }
            }
            return -1;
        },
        .setDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t colorBuffer) -> int {
            mMultiDisplay[displayId].cb = colorBuffer;
            return 0;
        }};

static bool sIsFolded = false;

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".getEmulatorWindow\n");
                    return (EmulatorWindow*)nullptr;
                },
        .rotate90Clockwise =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".rotate90Clockwise\n");
                    return true;
                },
        .rotate =
                [](SkinRotation rotation) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".rotate90Clockwise\n");
                    return true;
                },
        .getRotation =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .getRotation\n");
                    return SKIN_ROTATION_0;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .showMessage %s\n",
                            message);
                },
        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".showMessageWithDismissCallback %s\n",
                            message);
                },
        .fold =
                [](bool is_fold) -> bool {
                    fprintf(stderr, "window-agent-GfxStream-impl: .fold %d\n",
                            is_fold);
                    sIsFolded = is_fold;
                    return true;
                },
        .isFolded = [](void) -> bool {
            fprintf(stderr, "window-agent-GfxStream-impl: .isFolded ? %d\n",
                    sIsFolded);
            return sIsFolded;
        },
        .getFoldedArea = [](int* x, int* y, int* w, int* h) -> bool {
            fprintf(stderr, "window-agent-GfxStream-impl: .getFoldedArea\n");
            return true;
        },
        .updateFoldablePostureIndicator = [](bool) {
            fprintf(stderr, "window-agent-GfxStream-impl: updateFoldablePostureIndicator\n");
        },
        .setUIDisplayRegion =
                [](int x_offset, int y_offset, int w, int h) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .setUIDisplayRegion "
                            "%d %d %dx%d\n",
                            x_offset, y_offset, w, h);
                },
        .getMultiDisplay = 0,
        .setNoSkin = [](void) {},
        .restoreSkin = [](void) {},
        .updateUIMultiDisplayPage =
                [](uint32_t id) {
                    fprintf(stderr, "updateMultiDisplayPage\n");
                },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) -> bool {
                    if (w)
                        *w = 2500;
                    if (h)
                        *h = 1600;
                    return true;
                },
        .setResizableIcon =
                [](int presetSize) {},
};

class ToplevelConsoleFactory
: public android::emulation::AndroidConsoleFactory {
    const QAndroidVmOperations* const android_get_QAndroidVmOperations()
        const override {
            return &sQAndroidVmOperations;
        }

    const QAndroidMultiDisplayAgent* const
        android_get_QAndroidMultiDisplayAgent() const {
            return &sMultiDisplayAgent;
        }

    const QAndroidEmulatorWindowAgent* const
        android_get_QAndroidEmulatorWindowAgent()
        const {
            return &sQAndroidEmulatorWindowAgent;
        }

};

namespace aemu {

class Toplevel::Impl {
public:
    Impl(int refreshRate)
        : mRefreshRate(refreshRate),
          mUseWindow(System::get()->envGet("ANDROID_EMU_TEST_WITH_WINDOW") ==
                     "1"),
          mUseHostGpu(System::get()->envGet("ANDROID_EMU_TEST_WITH_HOST_GPU") ==
                      "1"),
          mDisplay(mUseWindow, kWindowSize, kWindowSize),
          mComposeWindow(kWindowSize, kWindowSize) {
        setupAndroidEmugl();
        setupGralloc();

        mComposeWindow.setProducer(&mToComposeWindow, &mFromComposeWindow);
    }

    ~Impl() {
        teardownDisplay();
        teardownGralloc();
        teardownAndroidEmugl();
    }

    AndroidWindow* createAppWindowAndSetCurrent(int width, int height) {
        AutoLock lock(mLock);
        if (!mSf) startDisplay();

        AndroidWindow* window = new AndroidWindow(width, height);

        mSf->connectWindow(window);
        mWindows.insert(window);

        return window;
    }

    void destroyAppWindow(AndroidWindow* window) {
        AutoLock lock(mLock);

        if (!window) return;
        if (mWindows.find(window) == mWindows.end()) return;

        if (mSf) {
            mSf->connectWindow(nullptr);
        }

        delete window;

        mWindows.erase(window);
    }

    void teardownDisplay() {
        if (!mSf) return;
        mSf->join();

        AutoLock lock(mLock);
        mSf.reset();

        for (auto buffer : mComposeBuffers) {
            destroyGrallocBuffer(buffer.handle);
        }

        for (auto buffer : mAppBuffers) {
            destroyGrallocBuffer(buffer.handle);
        }
    }

    void loop() {
        mDisplay.loop();
    }

private:

    void setupAndroidEmugl() {
        fprintf(stderr, "%s: inject\n", __func__);
        android::emulation::injectConsoleAgents(ToplevelConsoleFactory());
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
            android::featurecontrol::VulkanQueueSubmitWithCommands, true);
    android::featurecontrol::setEnabledOverride(
            android::featurecontrol::VulkanBatchedDescriptorSetUpdate, true);

        android_hw->hw_gltransport_asg_writeBufferSize = 262144;
        android_hw->hw_gltransport_asg_writeStepSize = 8192;
        android_hw->hw_gltransport_asg_dataRingSize = 131072;
        android_hw->hw_gltransport_drawFlushInterval = 800;

        EmuglConfig config;

        emuglConfig_init(
                &config, true /* gpu enabled */, "auto",
                mUseHostGpu ? "host"
                            : "swiftshader_indirect", /* gpu option */
                64,                                   /* bitness */
                mUseWindow, false,                    /* blacklisted */
                false,                                /* has guest renderer */
                WINSYS_GLESBACKEND_PREFERENCE_AUTO, false);

        emugl::vkDispatch(false /* not for test only */);

        emuglConfig_setupEnv(&config);

        // Set fake guest properties
        property_set("ro.kernel.qemu.gles", "1");
        property_set("qemu.sf.lcd_density", "420");
        property_set("ro.kernel.qemu.gltransport", "pipe");
        property_set("ro.kernel.qemu.gltransport.drawFlushInterval", "800");

        android_initOpenglesEmulation();

        int maj;
        int min;

        android_startOpenglesRenderer(kWindowSize, kWindowSize, 1, 28,
                                      getConsoleAgents()->vm,
                                      getConsoleAgents()->emu,
                                      getConsoleAgents()->multi_display,
                                      &maj, &min);

        char* vendor = nullptr;
        char* renderer = nullptr;
        char* version = nullptr;

        android_getOpenglesHardwareStrings(&vendor, &renderer, &version);

        printf("%s: GL strings; [%s] [%s] [%s].\n", __func__, vendor, renderer,
               version);

        if (mUseWindow) {
            android_showOpenglesWindow(mDisplay.getNative(), 0, 0, kWindowSize,
                                       kWindowSize, kWindowSize, kWindowSize,
                                       mDisplay.getDevicePixelRatio(), 0, false,
                                       false);
            mDisplay.loop();
        }

        android::emulation::goldfish_address_space_set_vm_operations(getConsoleAgents()->vm);

        HostAddressSpaceDevice::get();
        HostGoldfishPipeDevice::get();

        android_init_opengles_pipe();

        android_init_refcount_pipe();
    }

    void teardownAndroidEmugl() {
        AndroidPipe::Service::resetAll();
        android_finishOpenglesRenderer();
        android_hideOpenglesWindow();
        android_stopOpenglesRenderer(true);
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);
        set_global_gralloc_module(&mGralloc);
    }

    void teardownGralloc() { unload_gralloc_module(&mGralloc); }

    buffer_handle_t createGrallocBuffer(
            int usage = GRALLOC_USAGE_HW_RENDER,
            int format = HAL_PIXEL_FORMAT_RGBA_8888,
            int width = kWindowSize,
            int height = kWindowSize) {
        buffer_handle_t buffer;
        int stride;
        int res;

        res = mGralloc.alloc(width, height, format, usage, &buffer, &stride);
        if (res) {
            fprintf(stderr, "%s: alloc() failed with %s (%d)\n",
                    __func__, strerror(-res), -res);
            return nullptr;
        }
        res = mGralloc.registerBuffer(buffer);
        if (res) {
            fprintf(stderr, "%s: registerBuffer() failed with %s (%d)\n",
                    __func__, strerror(-res), -res);
            mGralloc.free(buffer);
            return nullptr;
        }

        (void)stride;

        return buffer;
    }

    void destroyGrallocBuffer(buffer_handle_t buffer) {
        int res;
        res = mGralloc.unregisterBuffer(buffer);
        if (res) {
            fprintf(stderr, "%s: unregisterBuffer() failed with %s (%d)\n",
                    __func__, strerror(-res), -res);
        }
        res = mGralloc.free(buffer);
        if (res) {
            fprintf(stderr, "%s: free() failed with %s (%d)\n",
                    __func__, strerror(-res), -res);
        }
    }

    std::vector<AndroidWindowBuffer> allocBuffersForQueue(int usage = GRALLOC_USAGE_HW_RENDER) {
        std::vector<AndroidWindowBuffer> buffers;
        for (int i = 0; i < AndroidBufferQueue::kCapacity; i++) {
            buffers.push_back(AndroidWindowBuffer(kWindowSize, kWindowSize,
                                                  createGrallocBuffer(usage)));
        }
        return buffers;
    }

    std::vector<ANativeWindowBuffer*> buffersToNativePtrs(
            const std::vector<AndroidWindowBuffer>& buffers) {
        std::vector<ANativeWindowBuffer*> res;

        for (int i = 0; i < buffers.size(); i++) {
            res.push_back((ANativeWindowBuffer*)(buffers.data() + i));
        }
        return res;
    }

    void startDisplay() {
        mAppBuffers = allocBuffersForQueue();
        mComposeBuffers = allocBuffersForQueue(GRALLOC_USAGE_HW_FB |
                                               GRALLOC_USAGE_HW_RENDER);

        auto nativeComposeBufferPtrs = buffersToNativePtrs(mComposeBuffers);

        for (auto buffer : nativeComposeBufferPtrs) {
            mToComposeWindow.queueBuffer(AndroidBufferQueue::Item(buffer));
        }

        mSf.reset(new SurfaceFlinger(
            mRefreshRate,
            &mComposeWindow,
            buffersToNativePtrs(mAppBuffers),
            [](AndroidWindow* composeWindow, AndroidBufferQueue* fromApp,
               AndroidBufferQueue* toApp) {

                return new ClientComposer(composeWindow, fromApp, toApp);

            },
            [this]() { mSf->advanceFrame(); this->clientPost(); }));

        mSf->start();
    }

    void clientPost() {
        AndroidBufferQueue::Item item = {};

        mFromComposeWindow.dequeueBuffer(&item);
        // ? mGralloc.post(item.buffer->handle);
        mToComposeWindow.queueBuffer(item);
    }

    Lock mLock;

    int mRefreshRate;

    bool mUseWindow;
    bool mUseHostGpu;

    Display mDisplay;

    AndroidWindow mComposeWindow;
    AndroidBufferQueue mToComposeWindow;
    AndroidBufferQueue mFromComposeWindow;

    struct gralloc_implementation mGralloc;

    std::vector<AndroidWindowBuffer> mAppBuffers;
    std::vector<AndroidWindowBuffer> mComposeBuffers;

    std::unique_ptr<SurfaceFlinger> mSf;

    std::unordered_set<AndroidWindow*> mWindows;
};

Toplevel::Toplevel(int refreshRate) : mImpl(new Toplevel::Impl(refreshRate)) {}
Toplevel::~Toplevel() = default;

ANativeWindow* Toplevel::createWindow(int width, int height) {
    return static_cast<ANativeWindow*>(mImpl->createAppWindowAndSetCurrent(width, height));
}

void Toplevel::destroyWindow(ANativeWindow* window) {
    mImpl->destroyAppWindow((AndroidWindow*) window);
}

void Toplevel::destroyWindow(void* window) {
    mImpl->destroyAppWindow((AndroidWindow*) window);
}

void Toplevel::teardownDisplay() {
    mImpl->teardownDisplay();
}

void Toplevel::loop() {
    mImpl->loop();
}
} // namespace aemu
