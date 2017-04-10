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
        decoder->media_type = AVMEDIA_TYPE_VIDEO;
    else
        decoder->media_type = AVMEDIA_TYPE_AUDIO;

    decoder->codec_id = codec_id;
    decoder->codec_context = codec_context;
    decoder->codec = codec;
    decoder->frame = frame;
    decoder->extradata = (char*)extradata;
    decoder->extradata_size = extradata_size;

    av_init_packet(&decoder->pkt);

    return decoder;
}

FFmpegDecoder::~FFmpegDecoder() {
    av_free(this->frame);
    avcodec_close(this->codec_context);
    av_free(this->codec_context);
    sws_freeContext(this->img_convert_ctx);

    if (this->extradata)
        free(this->extradata);
}

int FFmpegDecoder::decode(DisplayWindow* window,
                          const char* data,
                          int data_size) {
    if (data == NULL)
        return -1;

    if (this->media_type == AVMEDIA_TYPE_VIDEO) {
        decode_video(window, data, data_size, this->codec_context->width,
                     this->codec_context->height);
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
        this->pkt.data = (uint8_t*)data + data_size - remaining;
        this->pkt.size = remaining;

        len = avcodec_decode_video2(this->codec_context, this->frame, &decoded,
                                    &this->pkt);

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

            output_width = this->codec_context->width;
            output_height = this->codec_context->height;

/*
VERBOSE_PRINT(extdisplay, "linesize[0] %d linesize[1] %d linesize[2] %d
linesize[3] %d " "pix_fmt %d width %d height %d", this->frame->linesize[0],
this->frame->linesize[1], this->frame->linesize[2], this->frame->linesize[3],
        this->codec_context->pix_fmt,

        this->codec_context->width, this->codec_context->height);
*/

#ifdef _USE_SDL_

            SDLRenderer::draw_frame(window, this->frame);
            done = true;
#elif defined(ENABLE_GLES2)
            dest_pix_fmt = AV_PIX_FMT_YUV420P;
            avFrameRGB = av_frame_alloc();
            this->decoded_format = dest_pix_fmt;
            this->decoded_size = avpicture_get_size(dest_pix_fmt, output_width,
                                                    output_height);
            this->decoded_data = xzalloc(this->decoded_size);
            avpicture_fill((AVPicture*)avFrameRGB, this->decoded_data,
                           dest_pix_fmt, output_width, output_height);
            av_picture_copy((AVPicture*)avFrameRGB, (AVPicture*)this->frame,
                            dest_pix_fmt, output_width, output_height);
            update_window_yuv(window, avFrameRGB, avFrameRGB->linesize[0],
                              avFrameRGB->data[0], avFrameRGB->linesize[1],
                              avFrameRGB->data[1], avFrameRGB->linesize[2],
                              avFrameRGB->data[2], output_width, output_height);
            free(this->decoded_data);
            this->decoded_size = 0;

            av_free(avFrameRGB);

            if ((count % 100) == 0) {
                VERBOSE_PRINT(
                        extdisplay,
                        "decode_video2(%dms): size=%d len=%d picture_copy\n",
                        total_decode_time / count, this->pkt.size, len);
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
            if (!done && (this->codec_context->pix_fmt != dest_pix_fmt ||
                          output_width != this->codec_context->width ||
                          output_height != this->codec_context->height)) {
                if (this->img_convert_ctx == NULL) {
                    this->img_convert_ctx = sws_getContext(
                            this->codec_context->width,
                            this->codec_context->height,
                            this->codec_context->pix_fmt, output_width,
                            output_height, dest_pix_fmt, SWS_FAST_BILINEAR,
                            NULL, NULL, NULL);
                    if (this->img_convert_ctx == NULL) {
                        derror("Cannot initialize the conversion context!\n");
                    }
                }

                if (this->img_convert_ctx) {
                    avFrameRGB = av_frame_alloc();
                    this->decoded_format = dest_pix_fmt;
                    this->decoded_size = avpicture_get_size(
                            dest_pix_fmt, output_width, output_height);
                    this->decoded_data = (char*)xzalloc(this->decoded_size);
                    avpicture_fill((AVPicture*)avFrameRGB,
                                   (const uint8_t*)this->decoded_data,
                                   dest_pix_fmt, output_width, output_height);
                    sws_scale(this->img_convert_ctx, this->frame->data,
                              this->frame->linesize, 0,
                              this->codec_context->height, avFrameRGB->data,
                              avFrameRGB->linesize);

#ifndef _USE_SDL_
                    update_window(window, this->decoded_data,
                                  this->decoded_size, output_width,
                                  output_height);
#endif

                    free(this->decoded_data);
                    this->decoded_size = 0;

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
