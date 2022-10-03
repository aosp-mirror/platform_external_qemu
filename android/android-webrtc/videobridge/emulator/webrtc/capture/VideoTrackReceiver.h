// Copyright (C) 2021 The Android Open Source Project
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

#include <api/scoped_refptr.h>                            // for scoped_refptr
#include <chrono>                                         // for milliseconds

#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter
#include "api/video/video_frame_buffer.h"                 // for VideoFrameB...
#include "api/video/video_sink_interface.h"               // for VideoSinkIn...

namespace webrtc {
class VideoFrame;
}  // namespace webrtc

namespace emulator {
namespace webrtc {

using ::android::emulation::control::EventWaiter;
using ::webrtc::VideoFrame;
using ::webrtc::VideoFrameBuffer;
class VideoTrackReceiver : public rtc::VideoSinkInterface<VideoFrame> {
public:
    void OnFrame(const VideoFrame& frame);

    // Should be called by the source when it discards the frame due to rate
    // limiting.
    void OnDiscardedFrame();

    rtc::scoped_refptr<VideoFrameBuffer> currentFrame() { return mCurrentFrame; }

    // Wait at most timeout ms for the next frame. Returns the number of skipped frames.
    // 0 means no frame was received.
    int next(std::chrono::milliseconds timeout) { return frameWaiter.next(timeout); }

private:
    rtc::scoped_refptr<VideoFrameBuffer> mCurrentFrame;
    EventWaiter frameWaiter;
};
}  // namespace webrtc
}  // namespace emulator
