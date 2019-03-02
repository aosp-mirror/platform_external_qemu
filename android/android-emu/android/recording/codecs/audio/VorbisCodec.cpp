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

#include "android/recording/codecs/audio/VorbisCodec.h"

extern "C" {
#include "libavutil/opt.h"
}

#include "android/base/Log.h"
#include "android/base/system/System.h"

namespace android {
namespace recording {

VorbisCodec::VorbisCodec(CodecParams&& p,
                         AVSampleFormat inSampleFmt)
    : Codec(AV_CODEC_ID_VORBIS, std::move(p)),
      mInSampleFmt(inSampleFmt) {}

VorbisCodec::~VorbisCodec() {}

// Configures the encoder. Returns true if successful, false otherwise.
bool VorbisCodec::configAndOpenEncoder(const AVFormatContext* oc,
                                       AVCodecContext* c,
                                       AVStream* stream) const {
    stream->id = oc->nb_streams - 1;

    c->sample_fmt =
            mCodec->sample_fmts ? mCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = mParams.bitrate;
    c->sample_rate = mParams.sample_rate;

    if (mCodec->supported_samplerates) {
        c->sample_rate = mCodec->supported_samplerates[0];
        for (int i = 0; mCodec->supported_samplerates[i]; i++) {
            if (mCodec->supported_samplerates[i] == mParams.sample_rate) {
                c->sample_rate = mParams.sample_rate;
                break;
            }
        }
    }

    c->channel_layout = AV_CH_LAYOUT_STEREO;
    if (mCodec->channel_layouts) {
        c->channel_layout = mCodec->channel_layouts[0];
        for (int i = 0; mCodec->channel_layouts[i]; i++) {
            if (mCodec->channel_layouts[i] == AV_CH_LAYOUT_STEREO) {
                c->channel_layout = AV_CH_LAYOUT_STEREO;
                break;
            }
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

    // If you use a .WEBM container, the stream time base will automatically get changed
    // to a millisecond time base. Even so, let's explicitly set it anyways just in case
    // webm changes the format down the road.
    stream->time_base = (AVRational){1, 1000};
    // This time base should match up with the timebase we use when we get the timestamp
    // for the frames (getHighResTimeUs()). In this case, it's the microsecond time base.
    c->time_base = (AVRational){1, 1000000};

    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (mCodec->capabilities & CODEC_CAP_EXPERIMENTAL) {
        c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    // Open the codec
    AVDictionary* opts = nullptr;
    int ret = avcodec_open2(c, mCodec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        LOG(ERROR) << "Could not open audio codec (error code " << ret << ")";
        return false;
    }

    return true;
}

bool VorbisCodec::initSwxContext(const AVCodecContext* c,
                                       SwrContext* swxCxt) const {
    // set options for resampler context
    av_opt_set_int(swxCxt, "in_channel_count", c->channels, 0);
    av_opt_set_int(swxCxt, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(swxCxt, "in_sample_fmt", mInSampleFmt, 0);
    av_opt_set_int(swxCxt, "out_channel_count", c->channels, 0);
    av_opt_set_int(swxCxt, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(swxCxt, "out_sample_fmt", c->sample_fmt, 0);

    // initialize the resampling context
    if (swr_init(swxCxt) < 0) {
        LOG(ERROR) << "Failed to initialize the resampling context";
        return false;
    }

    return true;
}

}  // namespace recording
}  // namespace android
