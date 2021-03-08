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

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-videotoolbox-proxy-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif


namespace android {
namespace emulation {

using InitContextParam = H264PingInfoParser::InitContextParam;
using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
using ResetParam = H264PingInfoParser::ResetParam;
using GetImageParam = H264PingInfoParser::GetImageParam;
MediaH264DecoderVideoToolBoxProxy::MediaH264DecoderVideoToolBoxProxy(
        uint64_t id,
        H264PingInfoParser parser)
    : mId(id),
      mParser(parser),
      mFfmpegDecoder(id, parser),
      mVideoToolBoxDecoder(id, parser) {
    mCurrentDecoder = &mVideoToolBoxDecoder;
}

MediaH264DecoderPlugin* MediaH264DecoderVideoToolBoxProxy::clone() {
    return new MediaH264DecoderVideoToolBoxProxy(mId, mParser);
}

MediaH264DecoderVideoToolBoxProxy::~MediaH264DecoderVideoToolBoxProxy() {
    mCurrentDecoder->destroyH264Context();
    mCurrentDecoder = nullptr;
}

void MediaH264DecoderVideoToolBoxProxy::initH264Context(void* ptr) {
    mVideoToolBoxDecoder.initH264Context(ptr);
    mFfmpegDecoder.initH264Context(ptr);
}

void MediaH264DecoderVideoToolBoxProxy::reset(void* ptr) {
    mCurrentDecoder->reset(ptr);
}

std::vector<uint8_t> MediaH264DecoderVideoToolBoxProxy::prefixNaluHeader(std::vector<uint8_t> data) {
    std::vector<uint8_t> result {0x00, 0x00, 0x00,0x01};
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

void MediaH264DecoderVideoToolBoxProxy::decodeFrame(void* ptr) {
    if (mIsVideoToolBoxDecoderInGoodState) {
        mVideoToolBoxDecoder.decodeFrame(ptr);
        if (mVideoToolBoxDecoder.getState() == DecoderState::BAD_STATE) {
            mSPS = prefixNaluHeader(mVideoToolBoxDecoder.getSPS());
            mPPS = prefixNaluHeader(mVideoToolBoxDecoder.getPPS());
            // right now, the only place that videotoolbox can fail is when it gets SPS and PPS, and
            // failed to create decoding session
            mVideoToolBoxDecoder.destroyH264Context();
            mIsVideoToolBoxDecoderInGoodState = false;
            mFfmpegDecoder.decodeFrameDirect(ptr, &(mSPS[0]), mSPS.size(), 0);
            mFfmpegDecoder.decodeFrameDirect(ptr, &(mPPS[0]), mPPS.size(), 0);
            mCurrentDecoder = &mFfmpegDecoder;
        }
    } else {
        mFfmpegDecoder.decodeFrame(ptr);
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

void MediaH264DecoderVideoToolBoxProxy::save(base::Stream* stream) const {
    H264_DPRINT("saving ...");
    stream->putBe32(mParser.version());
    const int useHardwareDecoder = mIsVideoToolBoxDecoderInGoodState ? 1 : 0;
    stream->putBe32(useHardwareDecoder);
    mFfmpegDecoder.save(stream);
    mVideoToolBoxDecoder.save(stream);
}

bool MediaH264DecoderVideoToolBoxProxy::load(base::Stream* stream) {
    H264_DPRINT("loading ...");
    uint32_t version = stream->getBe32();
    mParser = H264PingInfoParser{version};
    const int useHardwareDecoder = stream->getBe32();
    mFfmpegDecoder.load(stream);
    mVideoToolBoxDecoder.load(stream);
    if (useHardwareDecoder) {
        mCurrentDecoder = &mVideoToolBoxDecoder;
    } else {
        mCurrentDecoder = &mFfmpegDecoder;
    }
    return true;
}

}  // namespace emulation
}  // namespace android
