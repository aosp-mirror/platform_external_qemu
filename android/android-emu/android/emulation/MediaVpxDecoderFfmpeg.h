// Copyright (C) 2019 The Android Open Source Project
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

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaHostRenderer.h"
#include "android/emulation/MediaVpxDecoderPlugin.h"
#include "android/emulation/VpxPingInfoParser.h"

extern "C" {
#include <libavcodec/avcodec.h>   // for AVCodecContext, AVPacket
#include <libavformat/avio.h>     // for avio_open, AVIO_FLAG_...
#include <libavutil/avutil.h>     // for AVMediaType, AVMEDIA_...
#include <libavutil/dict.h>       // for AVDictionary
#include <libavutil/error.h>      // for av_make_error_string
#include <libavutil/frame.h>      // for AVFrame, av_frame_alloc
#include <libavutil/log.h>        // for av_log_set_callback
#include <libavutil/pixfmt.h>     // for AVPixelFormat, AV_PIX...
#include <libavutil/rational.h>   // for AVRational
#include <libavutil/samplefmt.h>  // for AVSampleFormat
#include <libavutil/timestamp.h>
}

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaVpxDecoderFfmpeg : public MediaVpxDecoderPlugin {
public:
    virtual void initVpxContext(void* ptr) override;
    virtual void decodeFrame(void* ptr) override;
    virtual void getImage(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void destroyVpxContext(void* ptr) override;

    explicit MediaVpxDecoderFfmpeg(VpxPingInfoParser parser,
                                   MediaCodecType type);
    virtual ~MediaVpxDecoderFfmpeg();

    virtual int type() const override { return PLUGIN_TYPE_FFMPEG; }


public:
    void decodeFrameDirect(void* ptr,
                           const uint8_t* frame,
                           size_t szBytes,
                           uint64_t pts);

private:
    MediaCodecType mType;  // vp8 or vp9
    VpxPingInfoParser mParser;
    MediaHostRenderer mRenderer;

    void decodeFrameInternal(VpxPingInfoParser::DecodeFrameParam& param);

    void initVpxContextInternal();
    // image props
    int mNumDecodedFrame = 0;
    bool mImageReady = false;
    bool mIsInFlush = false;
    bool mFrameFormatChanged = false;
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    uint64_t mOutputPts = 0;
    uint64_t mInputPts = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = 0;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    unsigned int mOutBufferSize = 0;

    std::vector<uint8_t> mDecodedFrame;

    // ffmpeg stuff
    AVCodec* mCodec = nullptr;
    AVCodecContext* mCodecCtx = nullptr;
    AVFrame* mFrame = nullptr;
    AVPacket mPacket;

private:
    void copyFrame();
    void resetDecoder();
    bool checkWhetherConfigChanged(const uint8_t* frame, size_t szBytes);

};  // MediaVpxDecoderFfmpeg

}  // namespace emulation
}  // namespace android
