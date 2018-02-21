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
#include "android/base/memory/ScopedPtr.h"
#include "android/utils/debug.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <functional>
#include <memory>

#define SCALE_FLAGS SWS_BICUBIC

namespace android {
namespace recording {

namespace {

template <class T>
using CustomUniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

class GifConverterImpl {
public:
    explicit GifConverterImpl(const char* inFilename,
                              const char* outFilename,
                              uint32_t bitrate);

    bool wasSuccessful() { return mSucceeded; }

private:
    bool initialize(const char* inFilename,
                    const char* outFilename,
                    uint32_t bitrate);

    bool initInputContext(const char* inFilename);
    bool initOutputContext(const char* outFilename, uint32_t bitrate);
    bool initConversionContext();
    bool start();

    bool getNextVideoPacket(AVPacket* pkt);

    int encodeWriteFrame(AVFrame* filt_frame, int* got_frame);
    int flushEncoder();

private:
    bool mSucceeded = false;
    CustomUniquePtr<AVFormatContext> mInputContext;
    CustomUniquePtr<AVFormatContext> mOutputContext;
    CustomUniquePtr<AVStream> mOutputStream;
    CustomUniquePtr<SwsContext> mSwsContext;
    int mVideoStreamIndex = -1;
};

GifConverterImpl::GifConverterImpl(const char* inFilename,
                                   const char* outFilename,
                                   uint32_t bitrate) {
    if (initialize(inFilename, outFilename, bitrate)) {
        mSucceeded = start();
    }
}

bool GifConverterImpl::initialize(const char* inFilename,
                                  const char* outFilename,
                                  uint32_t bitrate) {
    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    av_register_all();
    return initInputContext(inFilename) &&
           initOutputContext(outFilename, bitrate);
}

bool GifConverterImpl::initInputContext(const char* inFilename) {
    int ret;

    // open the input video file
    AVFormatContext* ifmt_ctx = nullptr;
    if ((ret = avformat_open_input(&ifmt_ctx, inFilename, nullptr, nullptr)) <
        0) {
        LOG(ERROR) << "Cannot open input file: [" << inFilename << "]";
        return false;
    }

    // Open the codec for the input file.
    mInputContext = CustomUniquePtr<AVFormatContext>(
            std::move(ifmt_ctx), [this](AVFormatContext* ctx) {
                if (mVideoStreamIndex != -1) {
                    avcodec_close(ctx->streams[mVideoStreamIndex]->codec);
                }
                avformat_close_input(&ctx);
            });
    if ((ret = avformat_find_stream_info(mInputContext.get(), NULL)) < 0) {
        LOG(ERROR) << "Cannot find stream information";
        return false;
    }

    av_dump_format(mInputContext.get(), 0, inFilename, 0);

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
            mVideoStreamIndex = i;
            break;
        }
    }
    if (mVideoStreamIndex == -1) {
        LOG(ERROR) << "Cannot find video stream";
        return false;
    }

    return true;
}

bool GifConverterImpl::initOutputContext(const char* outFilename,
                                         uint32_t bitrate) {
    // open the output gif file
    AVFormatContext* ofmt_ctx = nullptr;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, outFilename);
    if (ofmt_ctx == nullptr) {
        LOG(ERROR) << "Could not create output context for [" << outFilename
                   << "]";
        return false;
    }
    mOutputContext = CustomUniquePtr<AVFormatContext>(
            std::move(ofmt_ctx), [](AVFormatContext* ctx) {
                if (ctx->oformat->flags & AVFMT_NOFILE) {
                    avio_closep(&ctx->pb);
                }
                avformat_free_context(ctx);
            });

    AVStream* out_stream = avformat_new_stream(mOutputContext.get(), nullptr);
    if (out_stream == NULL) {
        LOG(ERROR) << "Failed allocating output stream";
        return false;
    }
    mOutputStream =
            CustomUniquePtr<AVStream>(std::move(out_stream), [](AVStream* st) {
                if (st->codec) {
                    avcodec_close(st->codec);
                }
            });

    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (encoder == nullptr) {
        LOG(ERROR) << "GIF encoder not found";
        return false;
    }

    AVCodecContext* dec_ctx = mInputContext->streams[mVideoStreamIndex]->codec;
    AVCodecContext* enc_ctx = mOutputStream->codec;

    if ((encoder->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    enc_ctx->height = dec_ctx->height;
    enc_ctx->width = dec_ctx->width;
    enc_ctx->bit_rate = bitrate;
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    // take first format from list of supported formats, which should be
    // AV_PIX_FMT_RGB8
    enc_ctx->pix_fmt = encoder->pix_fmts[0];
    enc_ctx->time_base = dec_ctx->time_base;

    int ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
        LOG(ERROR) << "Cannot open video encoder for GIF";
        return false;
    }

    if (mOutputContext->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_dump_format(mOutputContext.get(), 0, outFilename, 1);

    if (!(mOutputContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&mOutputContext->pb, outFilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOG(ERROR) << "Could not open output file [" << outFilename << "]";
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

bool GifConverterImpl::start() {
    // init muxer, write output file header
    int ret = avformat_write_header(mOutputContext.get(), nullptr);
    if (ret < 0) {
        LOG(ERROR) << "Error occurred in avformat_write_header()";
        return false;
    }

    AVCodecContext* dec_ctx = mInputContext->streams[mVideoStreamIndex]->codec;
    AVCodecContext* enc_ctx = mOutputStream->codec;

    SwsContext* sws_ctx = nullptr;
    if (dec_ctx->pix_fmt != enc_ctx->pix_fmt) {
        sws_ctx = sws_getContext(enc_ctx->width, enc_ctx->height,
                                 dec_ctx->pix_fmt, enc_ctx->width,
                                 enc_ctx->height,
                                 /*AV_PIX_FMT_RGBA*/ enc_ctx->pix_fmt,
                                 SCALE_FLAGS, nullptr, nullptr, nullptr);
        if (sws_ctx == nullptr) {
            LOG(ERROR) << "Could not initialize the conversion context";
            return false;
        }
    }

    mSwsContext = CustomUniquePtr<SwsContext>(
            std::move(sws_ctx), [](SwsContext* ctx) { sws_freeContext(ctx); });

    // read all packets, decode, convert, then encode to gif format
    int64_t last_pts = -1;
    int got_frame;
    AVPacket packet = {0};
    packet.data = nullptr;
    packet.size = 0;
    while (getNextVideoPacket(&packet)) {
        auto pPacket = android::base::makeCustomScopedPtr(
                &packet, [](AVPacket* pkt) { av_packet_unref(pkt); });
        // for GIF, it seems we can't use a shared frame, so we have to alloc
        // it here inside the loop
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        auto pFrame = CustomUniquePtr<AVFrame>(
                std::move(frame), [](AVFrame* f) { av_frame_free(&f); });

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
            auto pTmpFrame = CustomUniquePtr<AVFrame>(
                    std::move(tmp_frame),
                    [](AVFrame* f) { av_frame_free(&f); });

            pTmpFrame->format = enc_ctx->pix_fmt;
            pTmpFrame->width = enc_ctx->width;
            pTmpFrame->height = enc_ctx->height;
            // allocate the buffer for the frame data
            ret = av_frame_get_buffer(pTmpFrame.get(), 32);
            if (ret < 0) {
                LOG(ERROR) << "Could not allocate video frame data.";
                break;
            }

            sws_scale(mSwsContext.get(), pFrame->data, pFrame->linesize, 0,
                      enc_ctx->height, pTmpFrame->data, pTmpFrame->linesize);
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

bool GifConverter::convertToAnimatedGif(const char* inFilename,
                                        const char* outFilename,
                                        uint32_t bitrate) {
    GifConverterImpl converter(inFilename, outFilename, bitrate);
    return converter.wasSuccessful();
}

}  // namespace recording
}  // namespace android

bool ffmpeg_convert_to_animated_gif(const char* input_video_file,
                                    const char* output_video_file,
                                    uint32_t gif_bit_rate) {
    return android::recording::GifConverter::convertToAnimatedGif(
            input_video_file, output_video_file, gif_bit_rate);
}
