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

#include "GoldfishOpenGLDispatch.h"
#include "GrallocDispatch.h"
#include "Standalone.h"

namespace emugl {

TEST(GoldfishOpenGLDispatchTest, Basic) {
    const DispatchTables* goldfishTables = loadGoldfishOpenGL();
    
    EXPECT_NE(nullptr, goldfishTables);
    
    // Check that loading it twice does not attempt to change the table
    // once it is loaded already
    EXPECT_EQ(goldfishTables, loadGoldfishOpenGL());

    // Check eglInitialize
    auto& egl = goldfishTables->egl;

    EGLDisplay display = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint maj, min;
    goldfishTables->egl.eglInitialize(display, &maj, &min);

    // Check that gralloc loads and we can open gralloc devices.
    const GrallocDispatch* gralloc = loadGoldfishGralloc();

    void *fb0Dev = gralloc->open_device("fb0");
    void *gpu0Dev = gralloc->open_device("gpu0");
}

} // namespace emugl
