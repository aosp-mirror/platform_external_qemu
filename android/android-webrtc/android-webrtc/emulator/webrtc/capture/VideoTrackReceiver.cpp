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
#include "VideoTrackReceiver.h"

#include <api/video/video_frame.h>         // for VideoFrame
#include <api/video/video_frame_buffer.h>  // for VideoFrameBuffer
#include <rtc_base/logging.h>              // for RTC_LOG
#include "aemu/base/Log.h"

namespace emulator {
namespace webrtc {
void VideoTrackReceiver::OnFrame(const VideoFrame& frame) {
    mCurrentFrame = frame.video_frame_buffer();
    frameWaiter.newEvent();
}

void VideoTrackReceiver::OnDiscardedFrame() {
    LOG(ERROR) << "Dropped frame.";
}

}  // namespace webrtc
}  // namespace emulator
