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
#include "VideoShareCapturer.h"

#include <string_view>

#include <api/video/video_frame.h>         // for VideoFrame, VideoFrame...
#include <api/video/video_frame_buffer.h>  // for VideoFrameBuffer
#include <api/video/video_rotation.h>      // for kVideoRotation_0
#include <rtc_base/logging.h>              // for RTC_LOG
#include <rtc_base/time_utils.h>           // for TimeMicros
#include <stddef.h>                        // for size_t
#include <string.h>                        // for memcpy

#include "aemu/base/Log.h"
#include "aemu/base/memory/SharedMemory.h"  // for SharedMemory, std::string_view
#include "api/video/i420_buffer.h"             // for I420Buffer
#include "libyuv/convert.h"                    // for ConvertToI420
#include "libyuv/rotate.h"                     // for kRotate0
#include "libyuv/video_common.h"               // for FOURCC_ARGB, FourCC

using android::base::SharedMemory;

namespace emulator {
namespace webrtc {

static const int kRGB8888BytesPerPixel = 4;

static bool openSharedMemory(SharedMemory& shm, std::string handle) {
    if (!shm.isOpen()) {
        int err = shm.open(SharedMemory::AccessMode::READ_ONLY);
        if (err != 0) {
            LOG(INFO) << "Unable to open memory mapped handle: ["
                            << handle << "] due to " << err;
            return false;
        }
    }

    return true;
}

VideoShareCapturer::VideoShareCapturer(std::string handle)
    : mSharedMemory("", 0) {
    // First read the memory settings.
    SharedMemory shm(handle, sizeof(VideoInfo));
    if (!openSharedMemory(shm, handle)) {
        mName = "bad_" + handle;
        return;
    }
    memcpy(&mVideoInfo, *shm, sizeof(VideoInfo));

    mPixelBufferSize = getBytesPerFrame(mVideoInfo);
    mSharedMemory = SharedMemory(handle, mPixelBufferSize + sizeof(VideoInfo));

    if (!openSharedMemory(mSharedMemory, handle)) {
        mName = "bad_" + handle;
        return;
    }

    mName = "shm_" + handle;
    mI420Buffer =
            ::webrtc::I420Buffer::Create(mVideoInfo.width, mVideoInfo.height);
    mPixelBuffer = (uint8_t*)*mSharedMemory + sizeof(VideoInfo);
}

size_t VideoShareCapturer::getBytesPerFrame(
        const VideoShareCapturer::VideoInfo& capability) {
    return (capability.width * capability.height * kRGB8888BytesPerPixel);
}

std::string VideoShareCapturer::name() const {
    return mName;
}

uint32_t VideoShareCapturer::maxFps() const {
    return mVideoInfo.fps;
}

bool VideoShareCapturer::isValid() const {
    return mSharedMemory.isOpen() && mVideoInfo.fps > 0;
}

int64_t VideoShareCapturer::frameNumber() {
    if (!isValid())
        return 0;

    VideoInfo* info = (VideoInfo*)*mSharedMemory;
    return info->frameNumber;
}

absl::optional<::webrtc::VideoFrame> VideoShareCapturer::getVideoFrame() {
    if (!isValid()) {
        return {};
    };

    auto converted = libyuv::ConvertToI420(
            mPixelBuffer, mPixelBufferSize, mI420Buffer.get()->MutableDataY(),
            mI420Buffer.get()->StrideY(), mI420Buffer.get()->MutableDataU(),
            mI420Buffer.get()->StrideU(), mI420Buffer.get()->MutableDataV(),
            mI420Buffer.get()->StrideV(),
            /*crop_x=*/0,
            /*crop_y=*/0, mVideoInfo.width, mVideoInfo.height, mVideoInfo.width,
            mVideoInfo.height, libyuv::kRotate0, libyuv::FourCC::FOURCC_ARGB);
    if (converted != 0) {
        LOG(INFO) << "Bad conversion frame." << frameNumber() << " "
                      << mVideoInfo.width << "x" << mVideoInfo.height;
        return {};
    }

    return ::webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(mI420Buffer)
            .set_timestamp_rtp(0)
            .set_timestamp_us(rtc::TimeMicros())
            .set_rotation(::webrtc::kVideoRotation_0)
            .build();
}

}  // namespace webrtc
}  // namespace emulator
