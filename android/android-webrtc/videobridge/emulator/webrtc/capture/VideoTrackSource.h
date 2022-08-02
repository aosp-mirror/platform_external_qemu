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
#pragma once

#include <absl/types/optional.h>                    // for optional
#include <api/media_stream_interface.h>             // for MediaSourceInterf...
#include <media/base/adapted_video_track_source.h>  // for AdaptedVideoTrack...
#include <rtc_base/platform_thread.h>               // for PlatformThread
#include <stdint.h>                                 // for uint32_t, int64_t

#include <atomic>  // for atomic_int
#include <memory>  // for unique_ptr

namespace rtc {
class PlatformThread;
}  // namespace rtc

namespace emulator {
namespace webrtc {
class VideoCapturer;

// A VideoTrackSource that provides video frames.
//
// A VideoTrackSource uses a VideoCapturer to provide images
// to the WebRTC engine. It does this by starting a thread that
// will obtain a frame from the VideoCapturer if a new frame is available.
class VideoTrackSource : public rtc::AdaptedVideoTrackSource {
public:
    explicit VideoTrackSource(VideoCapturer* capturer);
    ~VideoTrackSource() override;

    bool is_screencast() const override { return true; }
    absl::optional<bool> needs_denoising() const override { return false; }
    SourceState state() const override {
        return ::webrtc::MediaSourceInterface::SourceState::kLive;
    }
    bool remote() const override { return false; }
    void setFps(uint32_t fps);

private:
    static void captureThread(void* obj) {
        static_cast<VideoTrackSource*>(obj)->captureFrames();
    }

    void captureFrames();

    VideoCapturer* mVideoCapturer = nullptr;  // Not owned, can be null.
    uint64_t mPrevTimestamp = 0;
    bool mActive = false;

    rtc::PlatformThread mCaptureThread;
};
}  // namespace webrtc
}  // namespace emulator
