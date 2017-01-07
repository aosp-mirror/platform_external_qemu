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

#include "android/base/Log.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#define AUTO_RUN_GL_UNITTESTS 0

#if AUTO_RUN_GL_UNITTESTS
#define GLTEST(x, y) TEST(x, y)
#else
#define GLTEST(x, y) TEST(x, DISABLED_##y)
#endif

namespace emugl {

using android::base::Lock;
using android::base::AutoLock;
using namespace gltest;

GLTEST(OpenGL, Dispatch) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    EXPECT_TRUE(egl != nullptr);
    EXPECT_TRUE(gl != nullptr);
}

GLTEST(OpenGL, CreateContext) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    EXPECT_TRUE(egl != nullptr);
    EXPECT_TRUE(gl != nullptr);

    EGLDisplay dpy = getDisplay();
    EGLConfig config = createConfig(dpy, 8, 8, 8, 8, 24, 8, 0);
    EGLSurface surf = pbufferSurface(dpy, config, 512, 512);
    EGLContext context = createContext(dpy, config, 2, 0);

    destroyContext(dpy, context);
    destroySurface(dpy, surf);
    destroyDisplay(dpy);
}

}  // namespace emugl
