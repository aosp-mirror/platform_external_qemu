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
#include "VideoTrackSource.h"

#include <rtc_base/logging.h>          // for RTC_LOG
#include <rtc_base/platform_thread.h>  // for PlatformThread
#include <rtc_base/time_utils.h>       // for TimeMicros

#include <chrono>

#include "android/base/system/System.h"
#include "emulator/webrtc/capture/VideoCapturer.h"  // for VideoCapturer

namespace emulator {
namespace webrtc {

VideoTrackSource::VideoTrackSource(VideoCapturer* videoCapturer)
    : mVideoCapturer(videoCapturer) {
    if (mVideoCapturer == nullptr)
        return;

    mActive = true;
    mCaptureThread.reset(new rtc::PlatformThread(
            VideoTrackSource::captureThread, this, "VideoTrackSourceThread",
            rtc::kHighPriority));
    mCaptureThread->Start();
}

VideoTrackSource::~VideoTrackSource() {
    if (mActive) {
        mActive = false;
        mCaptureThread->Stop();
    }
}

void VideoTrackSource::captureFrames() {
    constexpr int64_t kUsPerSecond =
            std::chrono::microseconds(std::chrono::seconds(1)).count();
    constexpr int64_t kInitialDelivery =
            std::chrono::microseconds(std::chrono::seconds(5)).count();

    // Initially we deliver every individual frame, this guarantees that the
    // encoder pipeline will have frames and can generate keyframes for the web
    // endpoint, after a while we back down and only provide frames when we know
    // a frame has changed (b/139871418)
    auto deliverAllUntilTs = kInitialDelivery + rtc::TimeMicros();

    int64_t lastFrame = -1;
    while (mActive) {
        // Sleep up to mMaxFrameDelay, as we don't want to
        // slam the encoder.
        int64_t now = rtc::TimeMicros();
        auto maxFrameDelayUs = kUsPerSecond / mVideoCapturer->maxFps();
        int64_t sleeptime = maxFrameDelayUs - (now - mPrevTimestamp);
        if (sleeptime > 0) {
            android::base::System::get()->sleepUs(sleeptime);
        }

        auto currentFrame = mVideoCapturer->frameNumber();
        if (currentFrame > lastFrame || now < deliverAllUntilTs) {
            auto frame = mVideoCapturer->getVideoFrame();
            if (frame) {
                OnFrame(*frame);
            }
            lastFrame = currentFrame;
        }

        mPrevTimestamp = rtc::TimeMicros();
    }
}
}  // namespace webrtc
}  // namespace emulator
