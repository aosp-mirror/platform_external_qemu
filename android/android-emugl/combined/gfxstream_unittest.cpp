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
#include "OSWindow.h"
#include "android/base/system/System.h"

using android::base::System;

class GfxStreamBackendTest : public ::testing::Test {
private:
    static void sWriteFence(void* cookie, uint32_t fence) {
        uint32_t current = *(uint32_t*)cookie;
        if (current < fence)
            *(uint32_t*)(cookie) = fence;
    }

protected:
    uint32_t cookie;
    static const bool useWindow;
    struct virgl_renderer_callbacks callbacks;
    static constexpr uint32_t width = 256;
    static constexpr uint32_t height = 256;
    static std::unique_ptr<OSWindow> window;

    GfxStreamBackendTest()
        : cookie(0),
          callbacks({
                  0,
                  sWriteFence,
                  0,
                  0,
                  0,
          }) {}

    static void SetUpTestSuite() {
        if (useWindow) {
            window.reset(CreateOSWindow());
        }
    }

    static void TearDownTestSuite() { window.reset(nullptr); }

    void SetUp() override {
        System::setEnvironmentVariable("ANDROID_GFXSTREAM_EGL", "1");
        if (useWindow) {
            window->initialize("GfxStreamBackendTestWindow", width, height);
            window->setVisible(true);
            window->messageLoop();
        }
    }

    void TearDown() override {
        if (useWindow) {
            window->destroy();
        }
        gfxstream_backend_teardown();
    }
};

std::unique_ptr<OSWindow> GfxStreamBackendTest::window = nullptr;

const bool GfxStreamBackendTest::useWindow =
        System::get()->envGet("ANDROID_EMU_TEST_WITH_WINDOW") == "1";

TEST_F(GfxStreamBackendTest, Init) {
    gfxstream_backend_init(width, height, 0, &cookie,
                           GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT |
                                   GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT,
                           &callbacks);
}

TEST_F(GfxStreamBackendTest, InitOpenGLWindow) {
    if (!useWindow) {
        return;
    }
    gfxstream_backend_init(width, height, 0, &cookie,
                           GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT, &callbacks);
    gfxstream_backend_setup_window(window->getFramebufferNativeWindow(), 0, 0,
                                       width, height, width, height);
}

TEST_F(GfxStreamBackendTest, SimpleFlush) {
    gfxstream_backend_init(width, height, 0, &cookie,
                           GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT |
                                   GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT,
                           &callbacks);

    const uint32_t res_id = 8;
    struct virgl_renderer_resource_create_args create_resource_args = {
            .handle = res_id,
            .target = 2,  // PIPE_TEXTURE_2D
            .format = VIRGL_FORMAT_R8G8B8A8_UNORM,
            .bind = VIRGL_BIND_SAMPLER_VIEW | VIRGL_BIND_SCANOUT |
                    VIRGL_BIND_SHARED,
            .width = width,
            .height = height,
            .depth = 1,
            .array_size = 1,
            .last_level = 0,
            .nr_samples = 0,
            .flags = 0,
    };
    EXPECT_EQ(
            pipe_virgl_renderer_resource_create(&create_resource_args, NULL, 0),
            0);
    // R8G8B8A8 is used, so 4 bytes per pixel
    auto fb = std::make_unique<uint32_t[]>(width * height);
    EXPECT_NE(fb, nullptr);
    stream_renderer_flush_resource_and_readback(res_id, 0, 0, width, height,
                                                fb.get(), width * height);
}
