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

#include "android/recording/video/VideoFrameSharer.h"

#include <libyuv.h>
#include "android/base/Log.h"
#include "android/base/memory/SharedMemory.h"

namespace android {
namespace recording {

VideoFrameSharer::VideoFrameSharer(uint32_t fbWidth,
                                   uint32_t fbHeight,
                                   uint8_t fps,
                                   const std::string& handle)
    : mVideo({fbWidth, fbHeight, fps}),
      mMemory(handle, sizeof(mVideo) + getPixelBytes(mVideo)) {}

VideoFrameSharer::~VideoFrameSharer() {
    stop();
}

// VideoFormat --> libyuv representation
static int convertVideoType(VideoFormat video_type) {
    switch (video_type) {
        case VideoFormat::INVALID_FMT:
            return libyuv::FOURCC_ANY;
        case VideoFormat::RGB565:
            return libyuv::FOURCC_RGBP;
        case VideoFormat::RGBA8888:
            return libyuv::FOURCC_ABGR;
        case VideoFormat::BGRA8888:
            return libyuv::FOURCC_ARGB;
        default:
            LOG(FATAL) << "A new VideoFormat::... was introduced, please "
                          "update this translator";
    }
    return libyuv::FOURCC_ANY;
}

bool VideoFrameSharer::attachProducer(std::unique_ptr<Producer> producer) {
    // Prepare the shared memory with some basic info.
    const mode_t user_read_only = 0600;
    int err = mMemory.create(user_read_only);
    if (err != 0) {
        LOG(FATAL) << "Unable to open shared memory due to " << err;
        return false;
    }

    memcpy(mMemory.get(), &mVideo, sizeof(mVideo));

    mVideoProducer = std::move(producer);
    mVideoProducer->attachCallback([this](const Frame* frame) -> bool {
        return marshallFrame(frame);
    });
    mSourceFormat = convertVideoType(mVideoProducer->getFormat().videoFormat);
    return true;
}

void VideoFrameSharer::start() {
    if (mVideoProducer)
        mVideoProducer->start();
}

void VideoFrameSharer::stop() {
    if (mVideoProducer)
        mVideoProducer->stop();
}

bool VideoFrameSharer::marshallFrame(const Frame* frame) {
    uint8_t* bPixels = (uint8_t*)mMemory.get() + sizeof(mVideo);
    const int cPixelBytes = getPixelBytes(mVideo);
    // We need to convert this to I420, otherwise the WebRTC engine
    // will get confused. So we do that here.
    const size_t y_stride = mVideo.width;
    const size_t u_or_v_stride = y_stride / 2;

    const size_t y_size = y_stride * mVideo.height;
    const size_t u_or_v_size = u_or_v_stride * ((mVideo.height + 1) / 2);

    const size_t required_framebuffer_size = y_size + 2 * u_or_v_size;

    if (cPixelBytes < required_framebuffer_size) {
        LOG(FATAL) << "Staging framebuffer too small,"
                   << required_framebuffer_size << " bytes required,"
                   << cPixelBytes << " provided";
        return 0;
    }

    uint8_t* y_staging = bPixels;
    uint8_t* u_staging = y_staging + y_size;
    uint8_t* v_staging = u_staging + u_or_v_size;
    int result = ConvertToI420(frame->dataVec.data(),  // src_frame
                               cPixelBytes,            // src_size
                               y_staging,              // dst_y
                               y_stride,               // dst_stride_y
                               u_staging,              // dst_u
                               u_or_v_stride,          // dst_stride_u
                               v_staging,              // dst_v
                               u_or_v_stride,          // dst_stride_v
                               0,                      // crop_x
                               0,                      // crop_y
                               mVideo.width,           // src_width
                               mVideo.height,          // src_height
                               mVideo.width,           // crop_width
                               mVideo.height,          // crop_height
                               libyuv::kRotate0,       // rotation
                               mSourceFormat);         // source format.

    if (result != 0) {
        LOG(ERROR) << "Conversion failed!";
        return false;
    }
    return true;
}

size_t VideoFrameSharer::getPixelBytes(VideoInfo info) {
    // I420 = 12 bits per pixel.
    return info.width * info.height * 12 / 8;
}

}  // namespace recording
}  // namespace android
