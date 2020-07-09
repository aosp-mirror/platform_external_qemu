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

#include "android/emulation/MediaFfmpegVideoHelper.h"
#include "android/emulation/YuvConverter.h"
#include "android/utils/debug.h"

#define MEDIA_FFMPEG_DEBUG 0

#if MEDIA_FFMPEG_DEBUG
#define MEDIA_DPRINT(fmt, ...)                                              \
    fprintf(stderr, "media-ffmpeg-video-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define MEDIA_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using FrameInfo = MediaSnapshotState::FrameInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;

MediaFfmpegVideoHelper::MediaFfmpegVideoHelper(int type, int threads)
    : mType(type), mThreadCount(threads) {}

bool MediaFfmpegVideoHelper::init() {
    // standard ffmpeg codec stuff
    avcodec_register_all();
    bool print_all_decoder = false;
#if MEDIA_FFMPEG_DEBUG
    print_all_decoder = false;
#endif
    if (print_all_decoder) {
        AVCodec* current_codec = NULL;

        current_codec = av_codec_next(current_codec);
        while (current_codec != NULL) {
            if (av_codec_is_decoder(current_codec)) {
                MEDIA_DPRINT("codec decoder found %s long name %s",
                             current_codec->name, current_codec->long_name);
            }
            current_codec = av_codec_next(current_codec);
        }
    }

    AVCodecID codecType;
    switch (mType) {
        case 8:
            codecType = AV_CODEC_ID_VP8;
            MEDIA_DPRINT("create vp8 ffmpeg decoder%d", (int)mType);
            break;
        case 9:
            codecType = AV_CODEC_ID_VP9;
            MEDIA_DPRINT("create vp9 ffmpeg decoder%d", (int)mType);
            break;
        case 264:
            codecType = AV_CODEC_ID_H264;
            MEDIA_DPRINT("create h264 ffmpeg decoder%d", (int)mType);
            break;
        default:
            MEDIA_DPRINT("invalid codec %d", (int)mType);
            return false;
    }

    mCodec = NULL;
    mCodec = avcodec_find_decoder(codecType);
    if (!mCodec) {
        MEDIA_DPRINT("cannot find codec");
        return false;
    }

    mCodecCtx = avcodec_alloc_context3(mCodec);

    if (mThreadCount > 1) {
        mCodecCtx->thread_count = std::min(mThreadCount, 4);
        mCodecCtx->thread_type = FF_THREAD_FRAME;
        mCodecCtx->active_thread_type = FF_THREAD_FRAME;
    }
    avcodec_open2(mCodecCtx, mCodec, 0);
    mFrame = av_frame_alloc();

    MEDIA_DPRINT("Successfully created software h264 decoder context %p",
                 mCodecCtx);

    dprint("successfully created ffmpeg video decoder for %s",
           mType == 264 ? "H264" : (mType == 8 ? "VP8" : "VP9"));

    return true;
}

MediaFfmpegVideoHelper::~MediaFfmpegVideoHelper() {
    deInit();
}

void MediaFfmpegVideoHelper::deInit() {
    MEDIA_DPRINT("Destroy %p", this);
    mSavedDecodedFrames.clear();
    if (mCodecCtx) {
        avcodec_flush_buffers(mCodecCtx);
        avcodec_close(mCodecCtx);
        avcodec_free_context(&mCodecCtx);
        mCodecCtx = NULL;
        mCodec = NULL;
    }
    if (mFrame) {
        av_frame_free(&mFrame);
        mFrame = NULL;
    }
}

void MediaFfmpegVideoHelper::copyFrame() {
    int w = mFrame->width;
    int h = mFrame->height;
    mDecodedFrame.resize(w * h * 3 / 2);
    MEDIA_DPRINT("w %d h %d Y line size %d U line size %d V line size %d", w, h,
                 mFrame->linesize[0], mFrame->linesize[1], mFrame->linesize[2]);
    for (int i = 0; i < h; ++i) {
        memcpy(mDecodedFrame.data() + i * w,
               mFrame->data[0] + i * mFrame->linesize[0], w);
    }
    MEDIA_DPRINT("format is %d and NV21 is %d  NV12 is %d", mFrame->format,
                 (int)AV_PIX_FMT_NV21, (int)AV_PIX_FMT_NV12);
    if (mFrame->format == AV_PIX_FMT_NV12) {
        for (int i = 0; i < h / 2; ++i) {
            memcpy(w * h + mDecodedFrame.data() + i * w,
                   mFrame->data[1] + i * mFrame->linesize[1], w);
        }
        YuvConverter<uint8_t> convert8(w, h);
        convert8.UVInterleavedToPlanar(mDecodedFrame.data());
    } else {
        for (int i = 0; i < h / 2; ++i) {
            memcpy(w * h + mDecodedFrame.data() + i * w / 2,
                   mFrame->data[1] + i * mFrame->linesize[1], w / 2);
        }
        for (int i = 0; i < h / 2; ++i) {
            memcpy(w * h + w * h / 4 + mDecodedFrame.data() + i * w / 2,
                   mFrame->data[2] + i * mFrame->linesize[2], w / 2);
        }
    }
    MEDIA_DPRINT("copied Frame and it has presentation time at %lld",
                 (long long)(mFrame->pts));
}

void MediaFfmpegVideoHelper::flush() {
    MEDIA_DPRINT("flushing");
    avcodec_send_packet(mCodecCtx, NULL);
    fetchAllFrames();
    MEDIA_DPRINT("flushing done");
}

void MediaFfmpegVideoHelper::fetchAllFrames() {
    while (true) {
        int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
        if (retframe != 0) {
            MEDIA_DPRINT("no more frames");
            char tmp[1024];
            av_strerror(retframe, tmp, sizeof(tmp));
            MEDIA_DPRINT("WARNING: some unknown error %d: %s", retframe, tmp);
            return;

            break;
        }
        if (mIgnoreDecoderOutput) {
            continue;
        }
        copyFrame();
        MEDIA_DPRINT("save frame");
        mSavedDecodedFrames.push_back(MediaSnapshotState::FrameInfo{
                std::move(mDecodedFrame), std::vector<uint32_t>{},
                (int)mFrame->width, (int)mFrame->height, (uint64_t)mFrame->pts,
                ColorAspects{mFrame->color_primaries, mFrame->color_range,
                             mFrame->color_trc, mFrame->colorspace}});
    }
}

void MediaFfmpegVideoHelper::decode(const uint8_t* data,
                                    size_t len,
                                    uint64_t pts) {
    MEDIA_DPRINT("calling with size %d", (int)len);
    av_init_packet(&mPacket);
    mPacket.data = (unsigned char*)data;
    mPacket.size = len;
    mPacket.pts = pts;
    avcodec_send_packet(mCodecCtx, &mPacket);
    fetchAllFrames();
    MEDIA_DPRINT("done with size %d", (int)len);
}

}  // namespace emulation
}  // namespace android
