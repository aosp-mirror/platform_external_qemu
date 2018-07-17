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

//
// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "android/recording/GifConverter.h"

#include "android/base/Log.h"
#include "android/recording/AVScopedPtr.h"
#include "android/utils/debug.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <functional>
#include <memory>

static constexpr int SCALE_FLAGS = SWS_BICUBIC;

namespace android {
namespace recording {

namespace {

class GifConverterImpl {
public:
    explicit GifConverterImpl(android::base::StringView inFilename,
                              android::base::StringView outFilename,
                              uint32_t bitrate);
    bool run();

private:
    bool initialize(android::base::StringView inFilename,
                    android::base::StringView outFilename,
                    uint32_t bitrate);

    bool initInputContext(android::base::StringView inFilename);
    bool initOutputContext(android::base::StringView outFilename,
                           uint32_t bitrate);
    bool initConversionContext();

    bool getNextVideoPacket(AVPacket* pkt);

    int encodeWriteFrame(AVFrame* filt_frame, int* got_frame);
    int flushEncoder();

private:
    bool mIsValid = false;
    AVScopedPtr<AVFormatContext> mInputContext;
    AVScopedPtr<AVFormatContext> mOutputContext;
    AVScopedPtr<SwsContext> mSwsContext;
    AVCodecContext* mInVideoCodecCxt = nullptr;
    AVCodecContext* mOutVideoCodecCxt = nullptr;
    AVStream* mOutputStream = nullptr;
    int mVideoStreamIndex = -1;
};

GifConverterImpl::GifConverterImpl(android::base::StringView inFilename,
                                   android::base::StringView outFilename,
                                   uint32_t bitrate) {
    mIsValid = initialize(inFilename, outFilename, bitrate);
}

bool GifConverterImpl::initialize(android::base::StringView inFilename,
                                  android::base::StringView outFilename,
                                  uint32_t bitrate) {
    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();
    return initInputContext(inFilename) &&
           initOutputContext(outFilename, bitrate);
}

bool GifConverterImpl::initInputContext(android::base::StringView inFilename) {
    auto inFilenameValidated = android::base::c_str(inFilename);
    int ret;

    // open the input video file
    AVFormatContext* ifmt_ctx = nullptr;
    if ((ret = avformat_open_input(&ifmt_ctx, inFilenameValidated, nullptr,
                                   nullptr)) < 0) {
        LOG(ERROR) << "Cannot open input file: [" << inFilenameValidated << "]";
        return false;
    }
    mInputContext = makeAVScopedPtr(ifmt_ctx);

    // Open the codec for the input file.
    if ((ret = avformat_find_stream_info(mInputContext.get(), NULL)) < 0) {
        LOG(ERROR) << "Cannot find stream information";
        return false;
    }

    av_dump_format(mInputContext.get(), 0, inFilenameValidated, 0);

    // find the video stream, GIF supports only video, no audio
    for (int i = 0; i < mInputContext->nb_streams; i++) {
        AVStream* stream = mInputContext->streams[i];
        AVCodecContext* codec_ctx = stream->codec;

        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            // Open decoder
            ret = avcodec_open2(
                    codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                LOG(ERROR) << "Failed to open decoder for stream #" << i;
                return false;
            }
            mInVideoCodecCxt = codec_ctx;
            mVideoStreamIndex = i;
            break;
        }
    }
    if (!mInVideoCodecCxt) {
        LOG(ERROR) << "Cannot find video stream";
        return false;
    }

    return true;
}

bool GifConverterImpl::initOutputContext(android::base::StringView outFilename,
                                         uint32_t bitrate) {
    auto outFilenameValidated = android::base::c_str(outFilename);

    // open the output gif file
    AVFormatContext* ofmt_ctx = nullptr;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr,
                                   outFilenameValidated);
    if (ofmt_ctx == nullptr) {
        LOG(ERROR) << "Could not create output context for ["
                   << outFilenameValidated << "]";
        return false;
    }
    mOutputContext = makeAVScopedPtr(ofmt_ctx);

    AVStream* out_stream = avformat_new_stream(mOutputContext.get(), nullptr);
    if (out_stream == NULL) {
        LOG(ERROR) << "Failed allocating output stream";
        return false;
    }
    mOutputStream = out_stream;

    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (encoder == nullptr) {
        LOG(ERROR) << "GIF encoder not found";
        return false;
    }

    AVCodecContext* enc_ctx = mOutputStream->codec;

    if ((encoder->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    enc_ctx->height = mInVideoCodecCxt->height;
    enc_ctx->width = mInVideoCodecCxt->width;
    enc_ctx->bit_rate = bitrate;
    enc_ctx->sample_aspect_ratio = mInVideoCodecCxt->sample_aspect_ratio;
    // take first format from list of supported formats, which should be
    // AV_PIX_FMT_RGB8
    enc_ctx->pix_fmt = encoder->pix_fmts[0];
    enc_ctx->time_base = mInVideoCodecCxt->time_base;

    int ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
        LOG(ERROR) << "Cannot open video encoder for GIF";
        return false;
    }
    mOutVideoCodecCxt = enc_ctx;

    if (mOutputContext->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_dump_format(mOutputContext.get(), 0, outFilenameValidated, 1);

    if (!(mOutputContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&mOutputContext->pb, outFilenameValidated,
                        AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG(ERROR) << "Could not open output file [" << outFilenameValidated
                       << "]";
            return false;
        }
    }

    return true;
}

bool GifConverterImpl::getNextVideoPacket(AVPacket* pkt) {
    // Get next video packet
    while (av_read_frame(mInputContext.get(), pkt) >= 0) {
        if (pkt->stream_index != mVideoStreamIndex) {
            // Ignore audio packets
            continue;
        }
        av_packet_rescale_ts(
                pkt, mInputContext->streams[mVideoStreamIndex]->time_base,
                mInputContext->streams[mVideoStreamIndex]->codec->time_base);
        return true;
    }

    return false;
}

bool GifConverterImpl::run() {
    if (!mIsValid) {
        LOG(ERROR) << "Gif converter was given invalid parameters";
        return false;
    }

    // init muxer, write output file header
    int ret = avformat_write_header(mOutputContext.get(), nullptr);
    if (ret < 0) {
        LOG(ERROR) << "Error occurred in avformat_write_header()";
        return false;
    }

    SwsContext* sws_ctx = nullptr;
    if (mInVideoCodecCxt->pix_fmt != mOutVideoCodecCxt->pix_fmt) {
        sws_ctx = sws_getContext(mOutVideoCodecCxt->width, mOutVideoCodecCxt->height,
                                 mInVideoCodecCxt->pix_fmt, mOutVideoCodecCxt->width,
                                 mOutVideoCodecCxt->height,
                                 /*AV_PIX_FMT_RGBA*/ mOutVideoCodecCxt->pix_fmt,
                                 SCALE_FLAGS, nullptr, nullptr, nullptr);
        if (sws_ctx == nullptr) {
            LOG(ERROR) << "Could not initialize the conversion context";
            return false;
        }
    }
    mSwsContext = makeAVScopedPtr(sws_ctx);

    // read all packets, decode, convert, then encode to gif format
    int64_t last_pts = -1;
    int got_frame;
    AVPacket packet = {0};
    while (getNextVideoPacket(&packet)) {
        auto pPacket = makeAVScopedPtr(&packet);
        // for GIF, it seems we can't use a shared frame, so we have to alloc
        // it here inside the loop
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        auto pFrame = makeAVScopedPtr(frame);

        ret = avcodec_decode_video2(
                mInputContext->streams[mVideoStreamIndex]->codec, pFrame.get(),
                &got_frame, &packet);
        if (ret < 0) {
            LOG(ERROR) << "Decoding failed";
            break;
        }

        ret = 0;
        if (got_frame) {
            // Rescale pts to codec timebase
            pFrame->pts = av_frame_get_best_effort_timestamp(pFrame.get());
            // correct invalid pts in the rare cases

            // Need to check the scaled value if the output time_base has a
            // smaller resolution than the input time_base
            if (mOutputContext->streams[mVideoStreamIndex]
                        ->codec->time_base.den >
                mOutputContext->streams[mVideoStreamIndex]->time_base.den) {
                int64_t last_scaled_pts = -1;
                int64_t scaled_pts = av_rescale_q(
                        pFrame->pts,
                        mOutputContext->streams[mVideoStreamIndex]
                                ->codec->time_base,
                        mOutputContext->streams[mVideoStreamIndex]->time_base);

                if (last_pts > 0) {
                    last_scaled_pts = av_rescale_q(
                            last_pts,
                            mOutputContext->streams[mVideoStreamIndex]
                                    ->codec->time_base,
                            mOutputContext->streams[mVideoStreamIndex]
                                    ->time_base);
                }
                if (scaled_pts <= last_scaled_pts) {
                    pFrame->pts = av_rescale_q(
                            last_scaled_pts + 1,
                            mOutputContext->streams[mVideoStreamIndex]
                                    ->time_base,
                            mOutputContext->streams[mVideoStreamIndex]
                                    ->codec->time_base);
                }
            } else {
                if (pFrame->pts <= last_pts) {
                    pFrame->pts = last_pts + 1;
                }
            }
            last_pts = pFrame->pts;

            // Rescale the video pixel data
            AVFrame* tmp_frame = av_frame_alloc();
            if (!tmp_frame) {
                ret = AVERROR(ENOMEM);
                break;
            }
            auto pTmpFrame = makeAVScopedPtr(tmp_frame);

            pTmpFrame->format = mOutVideoCodecCxt->pix_fmt;
            pTmpFrame->width = mOutVideoCodecCxt->width;
            pTmpFrame->height = mOutVideoCodecCxt->height;
            // allocate the buffer for the frame data
            ret = av_frame_get_buffer(pTmpFrame.get(), 32);
            if (ret < 0) {
                LOG(ERROR) << "Could not allocate video frame data.";
                break;
            }

            sws_scale(mSwsContext.get(), pFrame->data, pFrame->linesize, 0,
                      mOutVideoCodecCxt->height, pTmpFrame->data, pTmpFrame->linesize);
            pTmpFrame->pts = pFrame->pts;
            ret = encodeWriteFrame(pTmpFrame.get(), nullptr);
            if (ret < 0) {
                break;
            }
        }
    }

    if (ret == AVERROR_EOF) {
        ret = 0;
    }

    if (ret == 0) {
        // flush encoder
        flushEncoder();
        av_write_trailer(mOutputContext.get());
    }

    // Close the output file and free the output context
    mOutputContext.reset();

    return ret == 0;
}

int GifConverterImpl::encodeWriteFrame(AVFrame* filt_frame, int* got_frame) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;

    if (!got_frame)
        got_frame = &got_frame_local;

    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = avcodec_encode_video2(
            mOutputContext->streams[mVideoStreamIndex]->codec, &enc_pkt,
            filt_frame, got_frame);
    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;

    // prepare packet for muxing
    enc_pkt.stream_index = mVideoStreamIndex;
    av_packet_rescale_ts(
            &enc_pkt,
            mOutputContext->streams[mVideoStreamIndex]->codec->time_base,
            mOutputContext->streams[mVideoStreamIndex]->time_base);
    // mux encoded frame
    // DO NOT free or unref enc_pkt once interleaved.
    // av_interleaved_write_frame() will take ownership of the packet once
    // passed in.
    ret = av_interleaved_write_frame(mOutputContext.get(), &enc_pkt);
    return ret;
}

// Gif conversion helper
// static
int GifConverterImpl::flushEncoder() {
    int ret;
    int got_frame;

    if (!(mOutputContext->streams[mVideoStreamIndex]
                  ->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n",
               mVideoStreamIndex);
        ret = encodeWriteFrame(nullptr, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }
    return ret;
}
}  // namespace

bool GifConverter::toAnimatedGif(android::base::StringView inFilename,
                                 android::base::StringView outFilename,
                                 uint32_t bitrate) {
    GifConverterImpl converter(inFilename, outFilename, bitrate);
    return converter.run();
}

}  // namespace recording
}  // namespace android
