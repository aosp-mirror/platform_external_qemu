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
#include "android/emulation/MediaH264Decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>                 // for AVCodecContext, AVPacket
#include <libavformat/avio.h>                   // for avio_open, AVIO_FLAG_...
#include <libavutil/avutil.h>                   // for AVMediaType, AVMEDIA_...
#include <libavutil/dict.h>                     // for AVDictionary
#include <libavutil/error.h>                    // for av_make_error_string
#include <libavutil/frame.h>                    // for AVFrame, av_frame_alloc
#include <libavutil/log.h>                      // for av_log_set_callback
#include <libavutil/pixfmt.h>                   // for AVPixelFormat, AV_PIX...
#include <libavutil/rational.h>                 // for AVRational
#include <libavutil/samplefmt.h>                // for AVSampleFormat
#include <libavutil/timestamp.h>
}

#include <stddef.h>
#include <mutex>
#include <unordered_map>

namespace android {
namespace emulation {

class MediaH264DecoderDefaultImpl;

class MediaH264DecoderDefault : public MediaH264Decoder {
public:
    MediaH264DecoderDefault() = default;
    virtual ~MediaH264DecoderDefault() = default;

    // This is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

private:
    std::mutex mIdLock{};
    std::mutex mMapLock{};
    uint64_t mId = 0;
    std::unordered_map<uint64_t, MediaH264DecoderDefaultImpl*> mDecoders;
    uint64_t readId(void* ptr);  // read id from the address
    void removeDecoder(uint64_t id);
    uint64_t createId();
    void addDecoder(uint64_t key,
                    MediaH264DecoderDefaultImpl* val);  // this just add
    void updateDecoder(
            uint64_t key,
            MediaH264DecoderDefaultImpl* val);  // this will overwrite
    MediaH264DecoderDefaultImpl* getDecoder(uint64_t key);
};

class MediaH264DecoderDefaultImpl {
public:
    MediaH264DecoderDefaultImpl();
    ~MediaH264DecoderDefaultImpl();

    using PixelFormat = MediaH264Decoder::PixelFormat;
    using Err = MediaH264Decoder::Err;

private:
    void initH264Context(unsigned int width,
                         unsigned int height,
                         unsigned int outWidth,
                         unsigned int outHeight,
                         PixelFormat pixFmt);
    void destroyH264Context();
    void decodeFrame(void* ptr,
                     const uint8_t* frame,
                     size_t szBytes,
                     uint64_t inputPts);
    void flush(void* ptr);
    void getImage(void* ptr);

    friend MediaH264DecoderDefault;

private:
    // image props
    bool mImageReady = false;
    bool mIsInFlush = false;
    bool mFrameFormatChanged = false;
    static constexpr int kBPP = 2; // YUV420 is 2 bytes per pixel
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mColorPrimaries = 2;
    unsigned int mColorRange = 0;
    unsigned int mColorTransfer = 2;
    unsigned int mColorSpace = 2;
    uint64_t mOutputPts = 0;
    uint64_t mInputPts = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = 0;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    PixelFormat mOutPixFmt;
    unsigned int mOutBufferSize = 0;

    // right now, decoding command only passes the input address;
    // and output address is only available in getImage().
    // TODO: this should be set to the output address to avoid
    // extra copying
    uint8_t *mDecodedFrame = nullptr;



    // ffmpeg stuff
    AVCodec *mCodec = nullptr;;
    AVCodecContext *mCodecCtx = nullptr;
    AVFrame *mFrame = nullptr;
    AVPacket mPacket;

private:
    void copyFrame();

};  // MediaH264DecoderDefault

}  // namespace emulation
}  // namespace android
