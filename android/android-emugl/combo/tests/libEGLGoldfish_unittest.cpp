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
#include "VirtualDisplay.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include <EGL/egl.h>

namespace emugl {

TEST(libEGLGoldfish, Basic) {
    aemu::initVirtualDisplay();
    auto& egl = loadGoldfishOpenGL()->egl;

    EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min;
    egl.eglInitialize(d, &maj, &min);

    void* drawElementsCall = egl.eglGetProcAddress("glDrawElements");
    fprintf(stderr, "%s: drawElements: %p\n", __func__, drawElementsCall);
}

} // namespace emugl

