// Copyright (C) 2020 The Android Open Source Project
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
#include "FrameBufferTestEnv.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/GLObjectCounter.h"
#include "android/base/perflogger/BenchmarkLibrary.h"
#include "android/base/system/System.h"
#include "android/emulation/control/window_agent.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include "GLSnapshotTesting.h"
#include "GLTestUtils.h"

#include <memory>

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

using android::base::System;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

namespace emugl {

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = nullptr,
        .rotate90Clockwise = nullptr,
        .rotate = nullptr,
        .getRotation = nullptr,
        .showMessage = nullptr,
        .showMessageWithDismissCallback = nullptr,
        .fold = nullptr,
        .isFolded = nullptr,
        .setUIDisplayRegion = [](int x,
                                 int y,
                                 int width,
                                 int height) {},
        .setUIMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              bool add,
                              uint32_t dpi = 0) {},
        .getMultiDisplay = nullptr,
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) {
                    if (w)
                        *w = 2500;
                    if (h)
                        *h = 1600;
                    return true;
                },
        .setNoSkin = []() {},
        .switchMultiDisplay = [](bool add,
                               uint32_t id,
                               int32_t x,
                               int32_t y,
                               uint32_t w,
                               uint32_t h,
                               uint32_t dpi,
                               uint32_t flag) { return true;},
        .updateUIMultiDisplayPage = [](uint32_t id) { },
};

extern "C" const QAndroidEmulatorWindowAgent* const
        gQAndroidEmulatorWindowAgent = &sQAndroidEmulatorWindowAgent;

FrameBufferTestEnv* setupFrameBufferTestEnv() {
    setupStandaloneLibrarySearchPaths();
    emugl::setGLObjectCounter(android::base::GLObjectCounter::get());
    emugl::set_emugl_window_operations(*gQAndroidEmulatorWindowAgent);
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

    bool useHostGpu = shouldUseHostGpu();
    const int xoffset = 400;
    const int yoffset = 400;
    const int width = 256;
    const int height = 256;

    OSWindow* window = createOrGetTestWindow(
        xoffset, yoffset, width, height);

    bool useSubWindow = window != nullptr;

    if (!FrameBuffer::initialize(
        width, height, useSubWindow, !useHostGpu /* egl2egl */)) {
        fprintf(stderr, "%s: fb failed to init\n", __func__);
        abort();
    }

    FrameBuffer* fb = FrameBuffer::getFB();

    if (useSubWindow) {
        fb->setupSubWindow(
            (FBNativeWindowType)(uintptr_t)
            window->getFramebufferNativeWindow(),
            0, 0,
            width, height, width, height,
            window->getDevicePixelRatio(), 0, false);
        window->messageLoop();
    }

    RenderThreadInfo* renderThreadInfo = new RenderThreadInfo();

    FrameBufferTestEnv* res = new FrameBufferTestEnv;
    res->useSubWindow = useSubWindow;
    res->window = window;
    res->fb = fb;
    res->renderThreadInfo = renderThreadInfo;
    res->width = width;
    res->height = height;
    res->xoffset = xoffset;
    res->yoffset = yoffset;

    return res;
}

void teardownFrameBufferTestEnv(FrameBufferTestEnv* env) {
    delete env->fb;
    delete env->renderThreadInfo;
    delete env;
}

} // namespace emugl
