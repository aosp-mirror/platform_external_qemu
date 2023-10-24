// Copyright (C) 2022 The Android Open Source Project
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
#include "emulator/webrtc/capture/InprocessVideoSource.h"

#include <api/video/video_frame.h>         // for VideoFrame::Builder
#include <api/video/video_frame_buffer.h>  // for VideoFrameBuffer
#include <api/video/video_rotation.h>      // for kVideoRotation_0
#include <rtc_base/logging.h>              // for RTC_LOG
#include <rtc_base/time_utils.h>           // for TimeMicros
#include <stdint.h>                        // for uint8_t

#include <algorithm>
#include <chrono>
#include <string>  // for string, operator==, stoi
#include <thread>  // for thread
#include <tuple>   // for make_tuple, tuple_elem...

#include "host-common/hw-config.h"
#include "aemu/base/EventNotificationSupport.h"
#include "aemu/base/Log.h"
#include "android/console.h"
#include "host-common/MultiDisplay.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/sensors_agent.h"  // for QAndroidSensors...
#include "android/emulation/control/utils/EventWaiter.h"
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/gpu_frame.h"
#include "host-common/opengles.h"
#include "api/video/i420_buffer.h"  // for I420Buffer
#include "libyuv/convert.h"         // for ConvertToI420
#include "libyuv/rotate.h"          // for kRotate0
#include "libyuv/video_common.h"    // for FOURCC_RGB3, FourCC

using android::base::RaiiEventListener;
using android::emulation::Image;
using android::emulation::ImageFormat;
using android::emulation::takeScreenshot;
using android::emulation::control::Callback;
using android::emulation::control::EventWaiter;
using android::emulation::control::Rotation;
using android::emulation::control::ScreenshotUtils;
namespace rtc {
struct VideoSinkWants;
template <typename VideoFrameT>
class VideoSinkInterface;
}  // namespace rtc

namespace emulator {
namespace webrtc {

InprocessVideoSource::InprocessVideoSource(int displayId)
    : mDisplayId(displayId) {}

void InprocessVideoSource::captureFrames() {
    LOG(DEBUG) << "Started capturing frame for display: " << mDisplayId;

    std::unique_ptr<EventWaiter> frameEvent;
    std::unique_ptr<
            RaiiEventListener<gfxstream::Renderer, gfxstream::FrameBufferChangeEvent>>
            frameListener;
    // Screenshots can come from either the gl renderer, or the guest.
    const auto& renderer = android_getOpenglesRenderer();
    if (renderer.get()) {
        // Fast mode..
        frameEvent = std::make_unique<EventWaiter>();
        frameListener = std::make_unique<RaiiEventListener<
                gfxstream::Renderer, gfxstream::FrameBufferChangeEvent>>(
                renderer.get(), [&](const gfxstream::FrameBufferChangeEvent state) {
                    frameEvent->newEvent();
                });
    } else {
        // slow mode, you are likely using older api..
        LOG(DEBUG) << "Reverting to slow callbacks";
        frameEvent = std::make_unique<EventWaiter>(
                [](Callback frameAvailable, void* opaque) {
                    gpu_register_shared_memory_callback(frameAvailable, opaque);
                },
                [](void* opaque) {
                    gpu_unregister_shared_memory_callback(opaque);
                });
    }

    const auto* agent = getConsoleAgents();
    if (!agent) {
        LOG(ERROR) << "Not able to retrieve console agent!";
        return;
    }
    uint32_t width = 0;
    uint32_t height = 0;
    bool enabled;
    bool multiDisplayQueryWorks =
            agent->emu->getMultiDisplay(mDisplayId, nullptr, nullptr, &width,
                                        &height, nullptr, nullptr, &enabled);

    // TODO(b/151387266): Use new query that uses a shared state for
    // multidisplay
    if (!multiDisplayQueryWorks) {
        LOG(DEBUG) << "Using the default width and height from display 0.";
        width = getConsoleAgents()->settings->hw()->hw_lcd_width;
        height = getConsoleAgents()->settings->hw()->hw_lcd_height;
    }

    mI420Buffer = ::webrtc::I420Buffer::Create(width, height);
    const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);
    // Video frames..
    ::webrtc::VideoFrame currentFrame =
            ::webrtc::VideoFrame::Builder()
                    .set_video_frame_buffer(mI420Buffer)
                    .set_timestamp_rtp(0)
                    .set_timestamp_us(rtc::TimeMicros())
                    .set_rotation(::webrtc::kVideoRotation_0)
                    .build();
    bool sendFirstFrame = true;
    while (mCaptureVideo == true) {
        // The next call will return the number of frames that are
        // available. 0 means no frame was made available in the given time
        // interval. Since this is a synchronous call we want to wait at
        // most kTimeToWaitForFrame so we can check if the client is still
        // there. (All clients get disconnected on emulator shutdown).
        auto arrived = frameEvent->next(kTimeToWaitForFrame);
        if (sendFirstFrame || (arrived > 0 && mCaptureVideo == true)) {
            sendFirstFrame = false;
            // TODO (wdu@) Add foldable support.
            SkinRect rect = {{0, 0}, {0, 0}};
            float xaxis = 0, yaxis = 0, zaxis = 0;
            auto out = {&xaxis, &yaxis, &zaxis};

            // TODO(jansene): Not clear if the rotation impacts displays other
            // than 0.
            /*agent->sensors->getPhysicalParameter(PHYSICAL_PARAMETER_ROTATION,
                                                 out.begin(), out.size(),
                                                 PARAMETER_VALUE_TYPE_CURRENT);*/
            auto rotation = ScreenshotUtils::deriveRotation(agent->sensors);
            if (rotation == Rotation::LANDSCAPE ||
                rotation == Rotation::REVERSE_LANDSCAPE) {
                std::swap(width, height);
            }

            android::emulation::Image img = takeScreenshot(
                    ImageFormat::RGB888, ScreenshotUtils::translate(rotation),
                    renderer.get(), agent->display->getFrameBuffer, mDisplayId,
                    width, height, rect);
            uint8_t* data = img.getPixelBuf();
            auto cData = img.getPixelCount();
            auto converted = libyuv::ConvertToI420(
                    data, cData, mI420Buffer.get()->MutableDataY(),
                    mI420Buffer.get()->StrideY(),
                    mI420Buffer.get()->MutableDataU(),
                    mI420Buffer.get()->StrideU(),
                    mI420Buffer.get()->MutableDataV(),
                    mI420Buffer.get()->StrideV(),
                    /*crop_x=*/0,
                    /*crop_y=*/0, img.getWidth(), img.getHeight(),
                    img.getWidth(), img.getHeight(), libyuv::kRotate0,
                    libyuv::FourCC::FOURCC_RGB3);
            currentFrame = ::webrtc::VideoFrame::Builder()
                                   .set_video_frame_buffer(mI420Buffer)
                                   .set_timestamp_rtp(0)
                                   .set_timestamp_us(rtc::TimeMicros())
                                   .set_rotation(::webrtc::kVideoRotation_0)
                                   .build();
            currentFrame.set_timestamp_us(rtc::TimeMicros());
            if (!currentFrame.video_frame_buffer()) {
                LOG(ERROR) << "currentFrame doesn't have a video frame buffer "
                              "attached!";
            } else {
                mBroadcaster.OnFrame(currentFrame);
            }
        }
    }
    LOG(DEBUG) << "Completed streaming frame for display: " << mDisplayId;
}

void InprocessVideoSource::cancel() {
    LOG(DEBUG) << "Cancelling video stream for display: " << mDisplayId;
    mCaptureVideo = false;
}

void InprocessVideoSource::run() {
    mCaptureVideo = true;
    captureFrames();
}

InprocessVideoSource::~InprocessVideoSource() {
    cancel();
}

bool InprocessVideoSource::GetStats(Stats* stats) {
    const std::lock_guard<std::mutex> lock(mStatsMutex);

    if (!mStats) {
        return false;
    }

    *stats = *mStats;
    return true;
}

void InprocessVideoSource::AddOrUpdateSink(
        rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink,
        const rtc::VideoSinkWants& wants) {
    mBroadcaster.AddOrUpdateSink(sink, wants);
    OnSinkWantsChanged(mBroadcaster.wants());
}

void InprocessVideoSource::RemoveSink(
        rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink) {
    mBroadcaster.RemoveSink(sink);
    OnSinkWantsChanged(mBroadcaster.wants());
}

void InprocessVideoSource::OnSinkWantsChanged(
        const rtc::VideoSinkWants& wants) {
    // TODO(jansene): Set the desired pixel count based on wants.
    LOG(WARNING) << "OnSinkWantsChanged desired pixels: "
                 << wants.target_pixel_count.value_or(0)
                 << ", max: " << wants.max_pixel_count
                 << ", fps: " << wants.max_framerate_fps
                 << ", align: " << wants.resolution_alignment;
    mVideoAdapter.OnSinkWants(wants);
}

}  // namespace webrtc
}  // namespace emulator
