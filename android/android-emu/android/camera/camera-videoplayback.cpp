/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "android/camera/camera-videoplayback.h"

#include "android/base/memory/LazyInstance.h"
#include "android/camera/camera-format-converters.h"
#include "android/camera/camera-videoplayback-render-multiplexer.h"

#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

#define VIDEOPLAYBACK_PIXEL_FORMAT V4L2_PIX_FMT_RGB32

namespace android {
namespace videoplayback {

using android::virtualscene::RenderedCameraDevice;

/*******************************************************************************
 *                     VideoPlaybackCameraDevice routines
 ******************************************************************************/

/*
 * Describes a connection to an actual camera device.
 */
class VideoPlaybackCameraDevice {
public:
    // Not copyable or movable.
    VideoPlaybackCameraDevice(const VideoPlaybackCameraDevice&) = delete;
    VideoPlaybackCameraDevice& operator=(const VideoPlaybackCameraDevice&) =
            delete;

    VideoPlaybackCameraDevice();

    CameraDevice* getCameraDevice() { return &mHeader; }
    RenderedCameraDevice* getMultiplexedCameraDevice() {
      return mRenderedCamera.get();
    }

private:
    // Common camera header.
    CameraDevice mHeader;
    std::unique_ptr<RenderedCameraDevice> mRenderedCamera;
};

VideoPlaybackCameraDevice::VideoPlaybackCameraDevice() {
    mHeader.opaque = this;
    auto renderer =
        std::unique_ptr<RenderMultiplexer>(new RenderMultiplexer());
    mRenderedCamera = std::unique_ptr<RenderedCameraDevice>(
        new RenderedCameraDevice(std::move(renderer)));
}
}  // namespace videoplayback
}  // namespace android

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

using android::videoplayback::DefaultFrameRenderer;
using android::videoplayback::VideoPlaybackCameraDevice;

static VideoPlaybackCameraDevice* toVideoPlaybackCameraDevice(
        CameraDevice* ccd) {
    if (!ccd || !ccd->opaque) {
        return nullptr;
    }

    return reinterpret_cast<VideoPlaybackCameraDevice*>(ccd->opaque);
}

uint32_t camera_videoplayback_preferred_format() {
    return VIDEOPLAYBACK_PIXEL_FORMAT;
}

CameraDevice* camera_videoplayback_open(const char* name, int inp_channel) {
    VideoPlaybackCameraDevice* cd = new VideoPlaybackCameraDevice();
    return cd ? cd->getCameraDevice() : nullptr;
}

int camera_videoplayback_start_capturing(CameraDevice* ccd,
                                         uint32_t pixel_format,
                                         int frame_width,
                                         int frame_height) {
    VideoPlaybackCameraDevice* cd = toVideoPlaybackCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    cd->getMultiplexedCameraDevice()->startCapturing(pixel_format, frame_width,
                                                     frame_height);

    return 0;
}

int camera_videoplayback_stop_capturing(CameraDevice* ccd) {
    VideoPlaybackCameraDevice* cd = toVideoPlaybackCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    cd->getMultiplexedCameraDevice()->stopCapturing();

    return 0;
}

int camera_videoplayback_read_frame(CameraDevice* ccd,
                                    ClientFrame* result_frame,
                                    float r_scale,
                                    float g_scale,
                                    float b_scale,
                                    float exp_comp) {
    VideoPlaybackCameraDevice* cd = toVideoPlaybackCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->getMultiplexedCameraDevice()->readFrame(
        result_frame, r_scale, g_scale, b_scale, exp_comp);
}

void camera_videoplayback_close(CameraDevice* ccd) {
    VideoPlaybackCameraDevice* cd = toVideoPlaybackCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return;
    }

    delete cd;
}
