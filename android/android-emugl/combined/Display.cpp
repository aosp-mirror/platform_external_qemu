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
#include "Display.h"

#include "AndroidWindow.h"
#include "GrallocDispatch.h"
#include "SurfaceFlinger.h"
#include "ClientComposer.h"

#include "android/base/files/PathUtils.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/emugl_config.h"

#include <atomic>

using android::AndroidPipe;
using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::Lock;
using android::base::pj;
using android::base::System;
using android::HostGoldfishPipeDevice;

namespace aemu {

class Vsync {
public:
    Vsync(int refreshRate = 60)
        : mRefreshRate(refreshRate),
          mRefreshIntervalUs(1000000ULL / mRefreshRate),
          mThread([this] {
              while (!mShouldStop.load(std::memory_order_relaxed)) {
                  System::get()->sleepUs(mRefreshIntervalUs);
                  AutoLock lock(mLock);
                  mSync = 1;
                  mCv.signal();
              }
              return 0;
          }) {
        mThread.start();
    }

    void join() {
        mShouldStop.store(true, std::memory_order_relaxed);
        mThread.wait();
    }

    ~Vsync() { join(); }

    void waitUntilNextVsync() {
        AutoLock lock(mLock);
        mSync = 0;
        while (!mSync) {
            mCv.wait(&mLock);
        }
    }

private:
    std::atomic<bool> mShouldStop{false};

    int mRefreshRate = 60;
    uint64_t mRefreshIntervalUs;
    volatile int mSync = 0;

    Lock mLock;
    ConditionVariable mCv;
    FunctorThread mThread;
};

// Starts up both the host and guest side graphics stack
class Display::Impl {
public:
    static constexpr int kWindowSize = 256;

    Impl() {
        setupHost();
        setupGralloc();

    }

    ~Impl() {
        teardownGralloc();
        teardownHost();
    }

    void setupHost() {
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

    }

    void teardownHost() {
        AndroidPipe::Service::resetAll();
        android_stopOpenglesRenderer(true);
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);
    }

    void teardownGralloc() {
        unload_gralloc_module(&mGralloc);
    }

private:
    struct gralloc_implementation mGralloc;
    Vsync mVsync;
};

Display::Display() : mImpl(new Display::Impl()) {}

Display::~Display() = default;

} // namespace aemu