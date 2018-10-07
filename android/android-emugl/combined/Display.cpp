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
#include "Display.h"

#include "OSWindow.h"

namespace aemu {

class Display::Impl {
public:
    Impl(bool useWindow, int width, int height) {
        if (useWindow) {
            window = CreateOSWindow();
            window->initialize("AndroidDisplay", width, height);
            window->setVisible(true);
            window->messageLoop();
        }
    }

    ~Impl() {
        if (window) window->destroy();
    }

    float getDevicePixelRatio() {
        if (window) return window->getDevicePixelRatio();
        return 1.0f;
    }

    void* getNative() {
        if (window) return window->getFramebufferNativeWindow();
        return nullptr;
    }

    void loop() {
        if (window) window->messageLoop();
    }

    OSWindow* window = nullptr;
};

Display::Display(bool useWindow, int width, int height)
    : mImpl(new Display::Impl(useWindow, width, height)) {}

Display::~Display() = default;

float Display::getDevicePixelRatio() {
    return mImpl->getDevicePixelRatio();
}

void* Display::getNative() {
    return mImpl->getNative();
}

void Display::loop() {
    mImpl->loop();
}

} // namespace aemu