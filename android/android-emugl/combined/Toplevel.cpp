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
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
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
#include "GrallocDispatch.h"
#include "Display.h"
#include "SurfaceFlinger.h"
#include "Vsync.h"
#include "VulkanDispatch.h"

#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

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
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLESDynamicVersion, true);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLAsyncSwap, false);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLDMA, false);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::RefCountPipe, true);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLDirectMem, true);
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::Vulkan, true);
        EmuglConfig config;

        emuglConfig_init(
                &config, true /* gpu enabled */, "auto",
                mUseHostGpu ? "host"
                            : "swiftshader_indirect", /* gpu option */
                64,                                   /* bitness */
                mUseWindow, false,                    /* blacklisted */
                false,                                /* has guest renderer */
                WINSYS_GLESBACKEND_PREFERENCE_AUTO);

        emugl::vkDispatch(false /* not for test only */);

        emuglConfig_setupEnv(&config);

        android_initOpenglesEmulation();

        int maj;
        int min;

        android_startOpenglesRenderer(kWindowSize, kWindowSize, 1, 28,
                                      gQAndroidVmOperations,
                                      gQAndroidEmulatorWindowAgent,
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
                                       mDisplay.getDevicePixelRatio(), 0, false);
            mDisplay.loop();
        }

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

        mGralloc.alloc(width, height, format, usage, &buffer, &stride);
        mGralloc.registerBuffer(buffer);

        (void)stride;

        return buffer;
    }

    void destroyGrallocBuffer(buffer_handle_t buffer) {
        mGralloc.unregisterBuffer(buffer);
        mGralloc.free(buffer);
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
        mGralloc.post(item.buffer->handle);
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
