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

#include "android/emulation/MediaH264DecoderVideoToolBoxProxy.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-videotoolbox-proxy-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif


namespace android {
namespace emulation {

MediaH264DecoderVideoToolBoxProxy::MediaH264DecoderVideoToolBoxProxy() {
    mCurrentDecoder = &mVideoToolBoxDecoder;
}

MediaH264DecoderVideoToolBoxProxy::~MediaH264DecoderVideoToolBoxProxy() {
    mCurrentDecoder->destroyH264Context();
    mCurrentDecoder = nullptr;
}

void MediaH264DecoderVideoToolBoxProxy::initH264Context(unsigned int width,
                                           unsigned int height,
                                           unsigned int outWidth,
                                           unsigned int outHeight,
                                           PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutputPixelFormat = outPixFmt;
    mCurrentDecoder->initH264Context(width, height, outWidth, outHeight, outPixFmt);
}

void MediaH264DecoderVideoToolBoxProxy::reset(unsigned int width,
                                           unsigned int height,
                                           unsigned int outWidth,
                                           unsigned int outHeight,
                                           PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutputPixelFormat = outPixFmt;
    mCurrentDecoder->reset(width, height, outWidth, outHeight, outPixFmt);
}

std::vector<uint8_t> MediaH264DecoderVideoToolBoxProxy::prefixNaluHeader(std::vector<uint8_t> data) {
    std::vector<uint8_t> result {0x00, 0x00, 0x00,0x01};
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

void MediaH264DecoderVideoToolBoxProxy::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes, uint64_t pts) {
    if (mIsVideoToolBoxDecoderInGoodState) {
        mVideoToolBoxDecoder.decodeFrame(ptr, frame, szBytes, pts);
        if (mVideoToolBoxDecoder.getState() == DecoderState::BAD_STATE) {
            mSPS = prefixNaluHeader(mVideoToolBoxDecoder.getSPS());
            mPPS = prefixNaluHeader(mVideoToolBoxDecoder.getPPS());
            // right now, the only place that videotoolbox can fail is when it gets SPS and PPS, and
            // failed to create decoding session
            assert(szBytes == mPPS.size());
            mVideoToolBoxDecoder.destroyH264Context();
            mIsVideoToolBoxDecoderInGoodState = false;
            mFfmpegDecoder.initH264Context(mWidth, mHeight, mOutputWidth, mOutputHeight, mOutputPixelFormat);
            mFfmpegDecoder.decodeFrame(ptr, &(mSPS[0]), mSPS.size(), 0);
            mFfmpegDecoder.decodeFrame(ptr, &(mPPS[0]), mPPS.size(), 0);
            mCurrentDecoder = &mFfmpegDecoder;
        }
    } else {
        mFfmpegDecoder.decodeFrame(ptr, frame, szBytes, pts);
    }
}

void MediaH264DecoderVideoToolBoxProxy::flush(void* ptr) {
    mCurrentDecoder->flush(ptr);
}

void MediaH264DecoderVideoToolBoxProxy::getImage(void* ptr) {
    mCurrentDecoder->getImage(ptr);
}

void MediaH264DecoderVideoToolBoxProxy::destroyH264Context() {
    mCurrentDecoder->destroyH264Context();
}

}  // namespace emulation
}  // namespace android
