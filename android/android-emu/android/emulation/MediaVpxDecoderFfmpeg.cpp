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

#include "android/emulation/MediaVpxDecoderFfmpeg.h"
#include "android/base/system/System.h"
#include "android/emulation/VpxPingInfoParser.h"
#include "android/emulation/YuvConverter.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt, ...)                                               \
    fprintf(stderr, "vpx-ffmpeg-dec: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using InitContextParam = VpxPingInfoParser::InitContextParam;
using DecodeFrameParam = VpxPingInfoParser::DecodeFrameParam;
using GetImageParam = VpxPingInfoParser::GetImageParam;
MediaVpxDecoderFfmpeg::~MediaVpxDecoderFfmpeg() {
    VPX_DPRINT("destroyed MediaVpxDecoderFfmpeg %p", this);
    destroyVpxContext(nullptr);
}

void MediaVpxDecoderFfmpeg::initVpxContext(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initVpxContextInternal();
}

void MediaVpxDecoderFfmpeg::initVpxContextInternal() {
    mIsInFlush = false;

    // standard ffmpeg codec stuff
    avcodec_register_all();
    if (1) {
        AVCodec* current_codec = NULL;

        current_codec = av_codec_next(current_codec);
        while (current_codec != NULL) {
            if (av_codec_is_decoder(current_codec)) {
                VPX_DPRINT("codec decoder found %s long name %s",
                           current_codec->name, current_codec->long_name);
            }
            current_codec = av_codec_next(current_codec);
        }
    }

    mCodec = avcodec_find_decoder(mType == MediaCodecType::VP8Codec
                                          ? AV_CODEC_ID_VP8
                                          : AV_CODEC_ID_VP9);
    if (!mCodec) {
        VPX_DPRINT("cannot find ffmpeg vpx decoder");
        return;
    }
    VPX_DPRINT("Using default software vpx decoder");
    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (!mCodecCtx) {
        VPX_DPRINT("cannot create ffmpeg vpx decoder context");
        return;
    }

    mCodecCtx->thread_count = 4;
    mCodecCtx->thread_type = FF_THREAD_FRAME;
    avcodec_open2(mCodecCtx, mCodec, 0);
    mFrame = av_frame_alloc();

    VPX_DPRINT("Successfully created software vpx decoder context %p",
               mCodecCtx);
}

MediaVpxDecoderFfmpeg::MediaVpxDecoderFfmpeg(VpxPingInfoParser parser,
                                             MediaCodecType type)
    : mParser(parser), mType(type) {
    VPX_DPRINT("allocated MediaVpxDecoderFfmpeg %p with version %d", this,
               (int)mParser.version());
}
void MediaVpxDecoderFfmpeg::destroyVpxContext(void* ptr) {
    VPX_DPRINT("Destroy context %p", this);
    if (mCodecCtx) {
        avcodec_close(mCodecCtx);
        av_free(mCodecCtx);
        mCodecCtx = NULL;
    }
    if (mFrame) {
        av_frame_free(&mFrame);
        mFrame = NULL;
    }
}

void MediaVpxDecoderFfmpeg::resetDecoder() {
    mNumDecodedFrame = 0;
    avcodec_close(mCodecCtx);
    av_free(mCodecCtx);
    mCodecCtx = avcodec_alloc_context3(mCodec);
    avcodec_open2(mCodecCtx, mCodec, 0);
}

void MediaVpxDecoderFfmpeg::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);
    decodeFrameInternal(param);
}

void MediaVpxDecoderFfmpeg::decodeFrameInternal(DecodeFrameParam& param) {
    const uint8_t* frame = param.p_data;
    size_t szBytes = param.size;
    uint64_t inputPts = param.user_priv;  // param.user_priv;

    VPX_DPRINT("%s(frame=%p, sz=%zu pts %lld)", __func__, frame, szBytes,
               (long long)inputPts);

    av_init_packet(&mPacket);
    mPacket.data = (unsigned char*)frame;
    mPacket.size = szBytes;
    mPacket.pts = inputPts;
    avcodec_send_packet(mCodecCtx, &mPacket);
    int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
    mIsInFlush = false;
    if (retframe != 0) {
        VPX_DPRINT("decodeFrame has nonzero return value %d", retframe);
        if (retframe == AVERROR_EOF) {
            VPX_DPRINT("EOF returned from decoder");
            VPX_DPRINT("EOF returned from decoder reset context now");
            resetDecoder();
        } else if (retframe == AVERROR(EAGAIN)) {
            VPX_DPRINT("EAGAIN returned from decoder");
        } else {
            VPX_DPRINT("unknown value %d", retframe);
        }
        return;
    }
    VPX_DPRINT("new w %d new h %d, old w %d old h %d", mFrame->width,
               mFrame->height, mOutputWidth, mOutputHeight);
    mFrameFormatChanged = false;
    ++mNumDecodedFrame;
    copyFrame();
    VPX_DPRINT("%s: got frame in decode mode", __func__);
    mImageReady = true;
}

void MediaVpxDecoderFfmpeg::copyFrame() {
    int w = mFrame->width;
    int h = mFrame->height;
    if (w != mOutputWidth || h != mOutputHeight) {
        mOutputWidth = w;
        mOutputHeight = h;
        mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
        mDecodedFrame.resize(mOutBufferSize);
    }
    VPX_DPRINT("w %d h %d Y line size %d U line size %d V line size %d", w, h,
               mFrame->linesize[0], mFrame->linesize[1], mFrame->linesize[2]);
    for (int i = 0; i < h; ++i) {
        memcpy(mDecodedFrame.data() + i * w,
               mFrame->data[0] + i * mFrame->linesize[0], w);
    }
    VPX_DPRINT("format is %d and NV21 is %d  12 is %d", mFrame->format,
               (int)AV_PIX_FMT_NV21, (int)AV_PIX_FMT_NV12);
    if (mFrame->format == AV_PIX_FMT_NV12) {
        for (int i = 0; i < h / 2; ++i) {
            memcpy(w * h + mDecodedFrame.data() + i * w,
                   mFrame->data[1] + i * mFrame->linesize[1], w);
        }
        YuvConverter<uint8_t> convert8(mOutputWidth, mOutputHeight);
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
    mOutputPts = mFrame->pts;
    VPX_DPRINT("copied Frame and it has presentation time at %lld",
               (long long)(mFrame->pts));
}

void MediaVpxDecoderFfmpeg::flush(void* ptr) {
    VPX_DPRINT("Flushing...");
    mIsInFlush = true;
    VPX_DPRINT("Flushing done");
}

void MediaVpxDecoderFfmpeg::getImage(void* ptr) {
    VPX_DPRINT("getImage %p", ptr);
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retErr = param.p_error;
    uint32_t* retWidth = param.p_d_w;
    uint32_t* retHeight = param.p_d_h;
    uint64_t* retPts = param.p_user_priv;

    // fixed the format
    constexpr int VPX_IMG_FMT_PLANAR = 0x100;
    constexpr int VPX_IMG_FMT_I420 = VPX_IMG_FMT_PLANAR | 2;
    *(param.p_fmt) = VPX_IMG_FMT_I420;

    static int numbers = 0;
    // VPX_DPRINT("calling getImage %d", numbers++);
    if (!mImageReady) {
        if (mFrameFormatChanged) {
            *retWidth = mOutputWidth;
            *retHeight = mOutputHeight;
            *retErr = 1;
            return;
        }
        if (mIsInFlush) {
            // guest be in flush mode, so try to get a frame
            avcodec_send_packet(mCodecCtx, NULL);
            int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
            if (retframe == AVERROR(EAGAIN) || retframe == AVERROR_EOF) {
                VPX_DPRINT("%s: frame is null", __func__);
                *retErr = 1;
                return;
            }

            if (retframe != 0) {
                char tmp[1024];
                av_strerror(retframe, tmp, sizeof(tmp));
                VPX_DPRINT("WARNING: some unknown error %d: %s", retframe, tmp);
                *retErr = 1;
                return;
            }
            VPX_DPRINT("%s: got frame in flush mode retrun code %d", __func__,
                       retframe);
            // now copy to mDecodedFrame
            copyFrame();
            mImageReady = true;
        } else {
            VPX_DPRINT("%s: no new frame yet", __func__);
            *retErr = 1;
            return;
        }
    }

    *retWidth = mOutputWidth;
    *retHeight = mOutputHeight;
    *retPts = mOutputPts;

    if (mParser.version() == 100) {
        uint8_t* dst = param.p_dst;
        memcpy(dst, mDecodedFrame.data(), mOutBufferSize);
    } else if (mParser.version() == 200) {
        if (param.hostColorBufferId >= 0) {
            mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                              mOutputWidth, mOutputHeight,
                                              mDecodedFrame.data());
        } else {
            uint8_t* dst = param.p_dst;
            memcpy(dst, mDecodedFrame.data(), mOutBufferSize);
        }
    }

    *retErr = 0;  // this means OK
    mImageReady = false;
    VPX_DPRINT("getImage %p done", ptr);
}

}  // namespace emulation
}  // namespace android
