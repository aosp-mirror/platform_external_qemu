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

#include <gtest/gtest.h>

#include "GfxStreamBackend.h"
#include "android/base/system/System.h"

using android::base::System;

static void sWriteFence(void* cookie, uint32_t fence) {
    uint32_t current = *(uint32_t*) cookie;
    if (current < fence) *(uint32_t*)(cookie) = fence;
}

TEST(GfxStreamBackend, Basic) {
    uint32_t cookie;
    struct virgl_renderer_callbacks cbs = {
        0,
        sWriteFence,
        0, 0, 0,

    };
    
    System::setEnvironmentVariable("ANDROID_GFXSTREAM_EGL", "1");
    gfxstream_backend_init(256, 256, 0, &cookie, GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT | GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT, &cbs);
    gfxstream_backend_teardown();
}