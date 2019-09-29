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
// VorbisCodec.h
//
//  Class to configure Vorbis codec
//

#pragma once

#include "android/recording/codecs/Codec.h"  // for Codec, CodecParams (ptr ...

extern "C" {
#include <libavcodec/avcodec.h>              // for AVCodecContext
#include <libavformat/avformat.h>            // for AVFormatContext, AVStream
#include <libavutil/samplefmt.h>             // for AVSampleFormat, AV_SAMPL...
#include "libswresample/swresample.h"        // for SwrContext
}

namespace android {
namespace recording {

class VorbisCodec : public Codec<SwrContext> {
public:
    explicit VorbisCodec(CodecParams&& params,
                         AVSampleFormat inSampleFmt);
    virtual ~VorbisCodec();

    // Configures the encoder. Returns true if successful, false otherwise.
    virtual bool configAndOpenEncoder(const AVFormatContext* oc,
                                      AVCodecContext* c,
                                      AVStream* stream) const override;
    // Configures and initializes the resampling context.
    virtual bool initSwxContext(const AVCodecContext* c,
                                SwrContext* swrCxt) const override;

protected:
    AVSampleFormat mInSampleFmt = AV_SAMPLE_FMT_NONE;
};

}  // namespace recording
}  // namespace android
