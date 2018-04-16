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

#include "android/recording/codecs/video/VP9Codec.h"

#include "android/base/Log.h"
#include "android/base/system/System.h"

#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC

namespace android {
namespace recording {

VP9Codec::VP9Codec(CodecParams&& p,
                               uint32_t fbWidth,
                               uint32_t fbHeight,
                               AVPixelFormat fbFormat)
    : Codec(AV_CODEC_ID_VP9, std::move(p)),
      mFbWidth(fbWidth),
      mFbHeight(fbHeight),
      mFbFormat(fbFormat) {}

VP9Codec::~VP9Codec() {}

// Configures the encoder. Returns true if successful, false otherwise.
bool VP9Codec::configAndOpenEncoder(const AVFormatContext* oc,
                                          AVStream* stream) const {
    stream->id = oc->nb_streams - 1;

    AVCodecContext* c = stream->codec;
    c->codec_id = mCodecId;
    c->bit_rate = mParams.bitrate;
    c->width = mParams.width;
    c->height = mParams.height;
    c->thread_count =
            std::min(8, android::base::System::get()->getCpuCoreCount() * 2);

    stream->time_base = (AVRational){1, static_cast<int>(mParams.fps)};
    c->time_base = stream->time_base;
    stream->avg_frame_rate = stream->time_base;

    // Key frame spacing
    c->gop_size = mParams.intra_spacing;
    c->pix_fmt = STREAM_PIX_FMT;

    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (mCodec->capabilities & CODEC_CAP_EXPERIMENTAL) {
        c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "deadline", "realtime", 0);
    av_dict_set(&opts, "cpu-used", "8", 0);

    // Open the codec
    int ret = avcodec_open2(c, mCodec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        LOG(ERROR) << "Could not open video codec (error code " << ret << ")";
        return false;
    }

    return true;
}

bool VP9Codec::initSwxContext(const AVCodecContext* c,
                                    SwsContext** swsCxt) const {
    *swsCxt =
            sws_getContext(mFbWidth, mFbHeight, mFbFormat, c->width, c->height,
                           c->pix_fmt, SCALE_FLAGS, NULL, NULL, NULL);
    if (*swsCxt == nullptr) {
        LOG(ERROR) << "Could not initialize the conversion context";
        return false;
    }

    return true;
}

}  // namespace recording
}  // namespace android
