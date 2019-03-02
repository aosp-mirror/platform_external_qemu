// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

///
// Codec.h
//
// Used to help configure codecs in FfmpegRecorder. Derive from this class and
// pass it to the recorder for any codecs you would like to use. See
// VP9Codec.h for an example.
//

#pragma once

#include <cassert>
#include <utility>

extern "C" {
#include "libavformat/avformat.h"
}

namespace android {
namespace recording {

// audio/video parameters to configure the codec. Feel free to add more if you
// need to.
struct CodecParams {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bitrate = 0;
    uint32_t sample_rate = 0;
    uint8_t fps = 0;
    uint8_t intra_spacing = 0;

    CodecParams() = default;
    CodecParams(CodecParams&& params) = default;
    CodecParams& operator=(CodecParams&& params) = default;
};

template <class T>
class Codec {
public:
    virtual ~Codec() {}

    // Configures the encoder and opens the codec context. Returns true if
    // successful, false otherwise.
    virtual bool configAndOpenEncoder(const AVFormatContext* oc,
                                      AVCodecContext* c,
                                      AVStream* stream) const = 0;
    // Configures and initializes either the SwsContext or SwrContext
    virtual bool initSwxContext(const AVCodecContext* c, T* swxCxt) const = 0;

    // Configures and initializes the resampling context
    // Returns the codec id.
    AVCodecID getCodecId() const { return mCodecId; }
    // Returns the codec.
    const AVCodec* getCodec() const { return mCodec; }
    // Returns the codec parameters
    const CodecParams* getCodecParams() const { return &mParams; }

protected:
    explicit Codec(AVCodecID id, CodecParams&& params)
        : mCodecId(id), mParams(std::move(params)) {
        mCodec = avcodec_find_encoder(mCodecId);
        assert(mCodec);
    }

protected:
    AVCodecID mCodecId = AV_CODEC_ID_NONE;
    CodecParams mParams;
    AVCodec* mCodec = nullptr;
};

}  // namespace recording
}  // namespace android
