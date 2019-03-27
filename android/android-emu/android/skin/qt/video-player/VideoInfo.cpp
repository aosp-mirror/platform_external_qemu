// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

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

#include "android/skin/qt/video-player/VideoInfo.h"

#include "android/base/memory/ScopedPtr.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayerRenderTarget.h"
#include "android/utils/debug.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

namespace android {
namespace videoplayer {

// VideoInfo implementations

VideoInfo::VideoInfo(VideoPlayerWidget* widget, std::string videoFile)
    : mVideoFile(videoFile), mWidget(widget) {
    initialize();
}

VideoInfo::~VideoInfo() {
    if (mPreviewFrame.buf != nullptr) {
        delete[] mPreviewFrame.buf;
    }
}

void VideoInfo::initialize() {
    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, mVideoFile.c_str(), NULL, NULL) != 0) {
        derror("Failed to open video file\n");
        return;  // failed to open video file
    }
    auto pFmtCtx = android::base::makeCustomScopedPtr(
            fmtCtx, [=](AVFormatContext* f) { avformat_close_input(&f); });

    if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        derror("Failed to find video stream info\n");
        return;  // failed to find stream info
    }

    // dump video format
    av_dump_format(fmtCtx, 0, mVideoFile.c_str(), false);

    // Find the first video stream
    int videoStreamIdx = -1;
    for (int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx == -1) {
        derror("Couldn't located the video stream\n");
        return;  // no audio or video stream found
    }

    mDurationSecs = calculateDurationSecs(fmtCtx);

    AVCodecContext* videoCodecCtx = fmtCtx->streams[videoStreamIdx]->codec;

    // Find the decoder for the video stream
    AVCodec* videoCodec = avcodec_find_decoder(videoCodecCtx->codec_id);
    if (videoCodec == nullptr) {
        derror("Unable to find the video decoder\n");
        return;
    }

    // Open codec
    if (avcodec_open2(videoCodecCtx, videoCodec, NULL) < 0) {
        derror("Unable to open the video codec\n");
        return;
    }
    auto pVideoCodecCtx = android::base::makeCustomScopedPtr(
            videoCodecCtx, [=](AVCodecContext* c) { avcodec_close(c); });

    // Calculate the dimensions for the video
    int dst_w;
    int dst_h;
    getDestinationSize(videoCodecCtx, mWidget, &dst_w, &dst_h);

    // create image convert context
    AVPixelFormat dst_fmt = AV_PIX_FMT_RGB24;
    SwsContext* imgConvertCtx = sws_getContext(
            videoCodecCtx->width, videoCodecCtx->height, videoCodecCtx->pix_fmt,
            dst_w, dst_h, dst_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (imgConvertCtx == nullptr) {
        derror("Could not allocate image convert context\n");
        return;
    }
    auto pImgConvertCtx = android::base::makeCustomScopedPtr(
            imgConvertCtx, [=](SwsContext* i) { sws_freeContext(i); });

    // Get the first valid packet from the video file
    int ret = 0;
    int got_frame = 0;
    AVPacket packet = {0};
    AVFrame* avframe = av_frame_alloc();
    if (avframe == nullptr) {
        derror("Unable to allocate AVFrame\n");
        return;
    }
    auto pavframe = android::base::makeCustomScopedPtr(
            avframe, [=](AVFrame* f) { av_frame_free(&f); });

    while ((ret = av_read_frame(fmtCtx, &packet)) >= 0) {
        if (packet.stream_index == videoStreamIdx) {
            if (packet.data != PacketQueue::sFlushPkt.data) {
                ret = 0;
                while (!got_frame) {
                    // Keep running decoder until we get the first frame.
                    // Decoder may not give us the first frame in the first
                    // decode call.
                    ret = avcodec_decode_video2(videoCodecCtx, avframe,
                                                &got_frame, &packet);
                    if (ret < 0) {
                        // Something went wrong
                        derror("Error while decoding frame\n");
                        return;
                    }
                }
                av_free_packet(&packet);
                break;
            }
        } else {
            // stream that we don't handle, simply free and ignore it
            av_free_packet(&packet);
        }
    }

    if (!got_frame) {
        // no valid frame
        derror("No valid frames were found\n");
        return;
    }

    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, dst_w, dst_h);
    int headerlen = 0;
    mPreviewFrame.buf = new unsigned char[numBytes + 64];
    // simply append a ppm header to become ppm image format
    headerlen = sprintf((char*)mPreviewFrame.buf, "P6\n%d %d\n255\n", dst_w, dst_h);
    mPreviewFrame.headerlen = headerlen;
    mPreviewFrame.len = numBytes + headerlen;

    // assign appropriate parts of buffer to image planes
    AVPicture pict;
    avpicture_fill(&pict, mPreviewFrame.buf + mPreviewFrame.headerlen, AV_PIX_FMT_RGB24,
                   dst_w, dst_h);

    // Convert the image to RGB format
    sws_scale(imgConvertCtx, avframe->data, avframe->linesize, 0,
              videoCodecCtx->height, pict.data, pict.linesize);
}

int VideoInfo::getDurationSecs() {
    return mDurationSecs;
}

// get the video size for the widget
void VideoInfo::getDestinationSize(AVCodecContext* c,
                                   VideoPlayerWidget* widget,
                                   int* pWidth,
                                   int* pHeight) {
    float aspect_ratio;

    if (c->sample_aspect_ratio.num == 0) {
        aspect_ratio = 0;
    } else {
        aspect_ratio = av_q2d(c->sample_aspect_ratio) * c->width / c->height;
    }

    if (aspect_ratio <= 0.0) {
        aspect_ratio = (float)c->width / (float)c->height;
    }

    widget->getRenderTargetSize(aspect_ratio, c->width, c->height, pWidth,
                                pHeight);
}

int VideoInfo::calculateDurationSecs(AVFormatContext* f) {
    // Code from av_dump_format to calulate the duration
    int64_t duration = f->duration + (f->duration <= INT64_MAX - 5000 ? 5000 : 0);
    return duration / AV_TIME_BASE;
}

// Show the frame
void VideoInfo::show() {
    VideoPlayerRenderTarget::FrameInfo info;
    info.headerlen = mPreviewFrame.headerlen;
    info.width = mPreviewFrame.width;
    info.height = mPreviewFrame.height;
    mWidget->setPixelBuffer(info, mPreviewFrame.buf, mPreviewFrame.len);
    emit updateWidget();
}

}  // namespace videoplayer
}  // namespace android
