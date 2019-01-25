/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/camera/camera-virtualscene.h"

#include "android/base/memory/LazyInstance.h"
#include "android/camera/camera-format-converters.h"
#include "android/camera/camera-virtualscene-utils.h"
#include "android/virtualscene/VirtualSceneManager.h"
#include "emugl/common/OpenGLDispatchLoader.h"

#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

#define VIRTUALSCENE_PIXEL_FORMAT V4L2_PIX_FMT_RGB32

namespace android {
namespace virtualscene {

class VirtualSceneRenderer : public CameraRenderer {
public:
    VirtualSceneRenderer() = default;
    ~VirtualSceneRenderer() = default;
    bool initialize(const GLESv2Dispatch* gles2,
                    int width,
                    int height) override {
        return VirtualSceneManager::initialize(gles2, width, height);
    }
    void uninitialize() override { VirtualSceneManager::uninitialize(); }
    int64_t render() override { return VirtualSceneManager::render(); }
};

}  // namespace virtualscene
}  // namespace android


/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

using android::virtualscene::RenderedCameraDevice;
using android::virtualscene::VirtualSceneManager;
using android::virtualscene::VirtualSceneRenderer;

static RenderedCameraDevice* toRenderedCameraDevice(CameraDevice* ccd) {
    if (!ccd || !ccd->opaque) {
        return nullptr;
    }

    return reinterpret_cast<RenderedCameraDevice*>(ccd->opaque);
}

void camera_virtualscene_parse_cmdline() {
    VirtualSceneManager::parseCmdline();
}

uint32_t camera_virtualscene_preferred_format() {
    return VIRTUALSCENE_PIXEL_FORMAT;
}

CameraDevice* camera_virtualscene_open(const char* name, int inp_channel) {
    RenderedCameraDevice* cd = new RenderedCameraDevice(
        std::unique_ptr<VirtualSceneRenderer>(new VirtualSceneRenderer()));
    return cd ? cd->getCameraDevice() : nullptr;
}

int camera_virtualscene_start_capturing(CameraDevice* ccd,
                                        uint32_t pixel_format,
                                        int frame_width,
                                        int frame_height) {
    RenderedCameraDevice* cd = toRenderedCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->startCapturing(pixel_format, frame_width, frame_height);
}

int camera_virtualscene_stop_capturing(CameraDevice* ccd) {
    RenderedCameraDevice* cd = toRenderedCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    cd->stopCapturing();

    return 0;
}

int camera_virtualscene_read_frame(CameraDevice* ccd,
                                   ClientFrame* result_frame,
                                   float r_scale,
                                   float g_scale,
                                   float b_scale,
                                   float exp_comp) {
    RenderedCameraDevice* cd = toRenderedCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->readFrame(result_frame, r_scale, g_scale, b_scale, exp_comp);
}

void camera_virtualscene_close(CameraDevice* ccd) {
    RenderedCameraDevice* cd = toRenderedCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return;
    }

    delete cd;
}
