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

#include <gtest/gtest.h>

#include "FrameBuffer.h"

#include "android/base/system/System.h"

#include "GLTestUtils.h"
#include "OpenGLTestContext.h"
#include "OSWindow.h"

using android::base::System;

namespace emugl {

using namespace gltest;

TEST_F(GLTest, FrameBufferBasic) {
    fprintf(stderr, "%s: creating window\n", __func__);
    OSWindow* window = CreateOSWindow();
    fprintf(stderr, "%s: created window\n", __func__);

    EXPECT_NE(nullptr, window);

    int width = 512;
    int height = 512;
    int x = 400;
    int y = 400;

    fprintf(stderr, "%s: initializing window window\n", __func__);
    EXPECT_TRUE(window->initialize("Window Test", width, height));
    window->setVisible(true);
    window->setPosition(x, y);
    EXPECT_NE(nullptr, window->getFramebufferNativeWindow());
    fprintf(stderr, "%s: initalized window\n", __func__);

    window->messageLoop();

    fprintf(stderr, "%s: initalizing fb\n", __func__);
    EXPECT_TRUE(FrameBuffer::initialize(width, height, true /* useSubWindow */, true /* egl2egl */));
    FrameBuffer* fb = FrameBuffer::getFB();
    EXPECT_NE(nullptr, fb);

    fprintf(stderr, "%s: setup sub window\n", __func__);
    fb->setupSubWindow((FBNativeWindowType)(uintptr_t)window->getFramebufferNativeWindow(), 0, 0, width, height, width, height, 1.0, 0, false);

    window->messageLoop();

    fprintf(stderr, "%s: created window\n", __func__);

    System::get()->sleepMs(5000);

    fprintf(stderr, "%s: exit window\n", __func__);

    window->destroy();
}

}  // namespace emugl
