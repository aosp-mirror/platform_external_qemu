// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/external-display/FFmpegDecoder.h"

#include "android/external-display/DisplayWindow.h"
#include "android/external-display/SDLRenderer.h"

#include "android/utils/debug.h"

#include <stdbool.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define _USE_SDL_

namespace android {
namespace display {

FFmpegDecoder* FFmpegDecoder::create(int is_vp8,
                                     const char* extradata,
                                     int extradata_size,
                                     int width,
                                     int height) {
    static bool g_registered = false;
    enum AVCodecID codec_id = (is_vp8) ? AV_CODEC_ID_VP8 : AV_CODEC_ID_H264;
    AVCodecContext* codec_context;
    AVCodec* codec = nullptr;
    AVFrame* frame;

    if (!g_registered) {
        avcodec_register_all();
        g_registered = true;
    }

    if (codec == nullptr)
        codec = avcodec_find_decoder(codec_id);
    if (!codec) {
        return nullptr;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        return nullptr;
    }

    if (codec->capabilities & CODEC_CAP_TRUNCATED)
        codec_context->flags |= CODEC_FLAG_TRUNCATED;

    codec_context->width = width;
    codec_context->height = height;
    codec_context->extradata = (uint8_t*)extradata;
    codec_context->extradata_size = extradata_size;

    frame = av_frame_alloc();

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        return nullptr;
    }

    FFmpegDecoder* decoder = new FFmpegDecoder();

    if (codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_VP8)
        decoder->mMediaType = AVMEDIA_TYPE_VIDEO;
    else
        decoder->mMediaType = AVMEDIA_TYPE_AUDIO;

    decoder->mCodecId = codec_id;
    decoder->mCodecContext = codec_context;
    decoder->mCodec = codec;
    decoder->mFrame = frame;
    decoder->mExtraData = (char*)extradata;
    decoder->mExtraDataSize = extradata_size;

    av_init_packet(&decoder->mPacket);

    return decoder;
}

FFmpegDecoder::~FFmpegDecoder() {
    av_free(this->mFrame);
    avcodec_close(this->mCodecContext);
    av_free(this->mCodecContext);
    sws_freeContext(this->mImgConvertCtx);

    if (this->mExtraData)
        free(this->mExtraData);
}

int FFmpegDecoder::decode(DisplayWindow* window,
                          const char* data,
                          int data_size) {
    if (data == NULL)
        return -1;

    if (this->mMediaType == AVMEDIA_TYPE_VIDEO) {
        decode_video(window, data, data_size, this->mCodecContext->width,
                     this->mCodecContext->height);
    } else {
        // here can handle audio decoding if needed
    }

    return 0;
}

/**
 * Allocate memory initialized to zero.
 * @param size
 */
void* FFmpegDecoder::xzalloc(size_t size) {
    void* mem;

    if (size < 1)
        size = 1;

    mem = calloc(1, size);

    if (mem == NULL) {
        derror("xzalloc: failed to allocate memory of size: %d\n", (int)size);
    }

    return mem;
}

bool FFmpegDecoder::decode_video(DisplayWindow* window,
                                 const char* data,
                                 int data_size,
                                 int output_width,
                                 int output_height) {
    int decoded;
    int len;
    bool ret = true;
    int remaining = data_size;

    while (remaining > 0) {
        this->mPacket.data = (uint8_t*)data + data_size - remaining;
        this->mPacket.size = remaining;

        len = avcodec_decode_video2(this->mCodecContext, this->mFrame, &decoded,
                                    &this->mPacket);

        if (len < 0) {
            VERBOSE_PRINT(extdisplay,
                          "data_size %d, avcodec_decode_video failed (%d)\n",
                          data_size, len);
            ret = false;
            break;
        } else if (!decoded) {
            ret = false;
            break;
        } else {
            AVFrame* avFrameRGB;
            bool done = false;
            enum AVPixelFormat dest_pix_fmt = AV_PIX_FMT_BGR24;

            output_width = this->mCodecContext->width;
            output_height = this->mCodecContext->height;

#ifdef _USE_SDL_

            SDLRenderer::draw_frame(window, this->mFrame);
            done = true;

#elif defined(ENABLE_GLES2)
            dest_pix_fmt = AV_PIX_FMT_YUV420P;
            avFrameRGB = av_frame_alloc();
            this->mDecodedFormat = dest_pix_fmt;
            this->mDecodedSize = avpicture_get_size(dest_pix_fmt, output_width,
                                                    output_height);
            this->mDecodedData = xzalloc(this->mDecodedSize);
            avpicture_fill((AVPicture*)avFrameRGB, this->mDecodedData,
                           dest_pix_fmt, output_width, output_height);
            av_picture_copy((AVPicture*)avFrameRGB, (AVPicture*)this->mFrame,
                            dest_pix_fmt, output_width, output_height);
            window->update_yuv(avFrameRGB, avFrameRGB->linesize[0],
                               avFrameRGB->data[0], avFrameRGB->linesize[1],
                               avFrameRGB->data[1], avFrameRGB->linesize[2],
                               avFrameRGB->data[2], output_width,
                               output_height);
            free(this->mDecodedData);
            this->mDecodedSize = 0;

            av_free(avFrameRGB);

            if ((count % 100) == 0) {
                VERBOSE_PRINT(
                        extdisplay,
                        "decode_video2(%dms): size=%d len=%d picture_copy\n",
                        total_decode_time / count, this->mPacket.size, len);
            }
            done = true;

#else

#endif

#ifdef _WIN32
            dest_pix_fmt = AV_PIX_FMT_BGR24;
#else
            dest_pix_fmt = AV_PIX_FMT_RGB24;
#endif

            // convert to RGB24 or BGR24
            if (!done && (this->mCodecContext->pix_fmt != dest_pix_fmt ||
                          output_width != this->mCodecContext->width ||
                          output_height != this->mCodecContext->height)) {
                if (this->mImgConvertCtx == NULL) {
                    this->mImgConvertCtx = sws_getContext(
                            this->mCodecContext->width,
                            this->mCodecContext->height,
                            this->mCodecContext->pix_fmt, output_width,
                            output_height, dest_pix_fmt, SWS_FAST_BILINEAR,
                            NULL, NULL, NULL);
                    if (this->mImgConvertCtx == NULL) {
                        derror("Cannot initialize the conversion context!\n");
                    }
                }

                if (this->mImgConvertCtx) {
                    avFrameRGB = av_frame_alloc();
                    this->mDecodedFormat = dest_pix_fmt;
                    this->mDecodedSize = avpicture_get_size(
                            dest_pix_fmt, output_width, output_height);
                    this->mDecodedData = (char*)xzalloc(this->mDecodedSize);
                    avpicture_fill((AVPicture*)avFrameRGB,
                                   (const uint8_t*)this->mDecodedData,
                                   dest_pix_fmt, output_width, output_height);
                    sws_scale(this->mImgConvertCtx, this->mFrame->data,
                              this->mFrame->linesize, 0,
                              this->mCodecContext->height, avFrameRGB->data,
                              avFrameRGB->linesize);

#ifndef _USE_SDL_
                    window->update(this->mDecodedData, this->mDecodedSize,
                                   output_width, output_height);
#endif

                    free(this->mDecodedData);
                    this->mDecodedSize = 0;

                    av_free(avFrameRGB);

                    // VERBOSE_PRINT(extdisplay, "sws_scale(%dms), decode(%dms),
                    // total_decode_time(%dms)\n", end_time - start_time,
                    // decode_time, total_decode_time);

                    done = true;
                }
            }

            remaining -= len;
            if (remaining)
                VERBOSE_PRINT(extdisplay, "remaining=%d\n", remaining);
        }
    }

    return ret;
}

}  // namespace display
}  // namespace android
