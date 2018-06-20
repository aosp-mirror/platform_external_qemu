// Copyright (C) 2017 The Android Open Source Project
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

#include "GLTestGlobals.h"

#include "android/base/system/System.h"
#include "android/base/memory/LazyInstance.h"

#include "emugl/common/misc.h"

using android::base::LazyInstance;
using android::base::System;

namespace gltest {

class TestWindow {
public:
    TestWindow() {
        window = CreateOSWindow();
    }

    ~TestWindow() {
        if (window) {
            window->destroy();
        }
    }

    // Check on initialization if windows are available.
    bool initializeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window->initialize("libOpenglRender test", width, height)) {
            window->destroy();
            window = nullptr;
            return false;
        }
        window->setVisible(true);
        window->setPosition(xoffset, yoffset);
        window->messageLoop();
        return true;
    }

    void resizeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window) return;

        window->setPosition(xoffset, yoffset);
        window->resize(width, height);
        window->messageLoop();
    }

    OSWindow* window = nullptr;
};

static LazyInstance<TestWindow> sTestWindow = LAZY_INSTANCE_INIT;

bool shouldUseHostGpu() {
    bool useHost = System::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    // Also set the global emugl renderer accordingly.
    if (useHost) {
        emugl::setRenderer(SELECTED_RENDERER_HOST);
    } else {
        emugl::setRenderer(SELECTED_RENDERER_SWIFTSHADER_INDIRECT);
    }

    return useHost;
}

bool shouldUseWindow() {
    bool useWindow = System::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_WINDOW") == "1";
    return useWindow;
}

OSWindow* createOrGetTestWindow(int xoffset, int yoffset, int width, int height) {
    if (!shouldUseWindow()) return nullptr;

    if (!sTestWindow.hasInstance()) {
        sTestWindow.get();
        sTestWindow->initializeWithRect(xoffset, yoffset, width, height);
    } else {
        sTestWindow->resizeWithRect(xoffset, yoffset, width, height);
    }
    return sTestWindow->window;
}

} // namespace gltest
