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

#include "android/emulation/MediaH264DecoderDefault.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {


MediaH264DecoderDefault::~MediaH264DecoderDefault() {
    H264_DPRINT("destroyed MediaH264DecoderDefault %p", this);
    destroyH264Context();
}

void MediaH264DecoderDefault::handlePing(MediaCodecType type, MediaOperation op, void* ptr) {
    switch (op) {
        case MediaOperation::InitContext:
        {
            uint8_t* xptr = (uint8_t*)ptr;
            unsigned int width = *(unsigned int*)(xptr);
            unsigned int height = *(unsigned int*)(xptr + 8);
            unsigned int outWidth = *(unsigned int*)(xptr + 16);
            unsigned int outHeight = *(unsigned int*)(xptr + 24);
            PixelFormat pixFmt = static_cast<PixelFormat>(*(uint8_t*)(xptr + 30));
            initH264Context(width, height, outWidth, outHeight, pixFmt);
            break;
        }
        case MediaOperation::DestroyContext:
            destroyH264Context();
            break;
        case MediaOperation::DecodeImage:
        {
            uint64_t offset = *(uint64_t*)(ptr);
            const uint8_t* img = (const uint8_t*)ptr + offset;
            size_t szBytes = *(size_t*)((uint8_t*)ptr + 8);
            mInputPts = *(size_t*)((uint8_t*)ptr + 16);
            decodeFrame(ptr, img, szBytes);
            break;
        }
        case MediaOperation::Flush:
            flush(ptr);
            break;
        case MediaOperation::GetImage:
            getImage(ptr);
            break;
        default:
            H264_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaH264DecoderDefault::initH264Context(unsigned int width,
                             unsigned int height,
                             unsigned int outWidth,
                             unsigned int outHeight,
                             PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;
    mOutBufferSize = outWidth * outHeight * 3 / 2;

    mIsInFlush = false;

    if (mDecodedFrame) {
      delete [] mDecodedFrame;
    }

    mDecodedFrame = new uint8_t[mOutBufferSize];

    // standard ffmpeg codec stuff
    avcodec_register_all();
    mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    mCodecCtx = avcodec_alloc_context3(mCodec);

    avcodec_open2(mCodecCtx, mCodec, 0);
    mFrame = av_frame_alloc();

    H264_DPRINT("Successfully created software h264 decoder context %p", mCodecCtx);
}

MediaH264DecoderDefault::MediaH264DecoderDefault() {
    H264_DPRINT("allocated MediaH264DecoderDefault %p", this);
}
void MediaH264DecoderDefault::destroyH264Context() {
    H264_DPRINT("Destroy context %p", this);
    if (mCodecCtx) {
        avcodec_close(mCodecCtx);
        av_free(mCodecCtx);
        mCodecCtx = NULL;
    }
    if (mFrame) {
        av_frame_free(&mFrame);
        mFrame = NULL;
    }
    if (mDecodedFrame) {
      delete [] mDecodedFrame;
      mDecodedFrame = nullptr;
    }
}

static void* getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void MediaH264DecoderDefault::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) {
    H264_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);
    Err h264Err = Err::NoErr;
    // TODO: move this somewhere else
    // First return parameter is the number of bytes processed,
    // Second return parameter is the error code
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    uint64_t* retSzBytes = (uint64_t*)retptr;
    int32_t* retErr = (int32_t*)(retptr + 8);

    av_init_packet(&mPacket);
    mPacket.data = (unsigned char*)frame;
    mPacket.size = szBytes;
    mPacket.pts = mInputPts;
    avcodec_send_packet(mCodecCtx, &mPacket);
    int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
    *retSzBytes = szBytes;
    *retErr = (int32_t)h264Err;
    if (retframe != 0) {
        H264_DPRINT("decodeFrame has nonzero return value %d", retframe);
        if (retframe == AVERROR_EOF) {
            H264_DPRINT("EOF returned from decoder");
            H264_DPRINT("EOF returned from decoder reset context now");
            avcodec_close(mCodecCtx);
            av_free(mCodecCtx);
            mCodecCtx = avcodec_alloc_context3(mCodec);
            avcodec_open2(mCodecCtx, mCodec, 0);
            mIsInFlush = false;
        } else if (retframe == AVERROR(EAGAIN)) {
            H264_DPRINT("EAGAIN returned from decoder");
        } else {
            H264_DPRINT("unknown value %d", retframe);
        }
        return;
    }
    H264_DPRINT("new w %d new h %d, old w %d old h %d",
                mFrame->width, mFrame->height,
                mOutputWidth, mOutputHeight);
    mFrameFormatChanged = false;
    if (mFrame->width != mOutputWidth || mFrame->height != mOutputHeight) {
        mOutputHeight = mFrame->height;
        mOutputWidth = mFrame->width;
        mFrameFormatChanged = true;
        H264_DPRINT("%s: does not got frame in decode mode, format changed", __func__);
        *retErr = static_cast<int>(Err::DecoderRestarted);
        return;
    }
    copyFrame();
    mOutputPts = mFrame->pts;
    H264_DPRINT("%s: got frame in decode mode", __func__);
    mImageReady = true;
}

void MediaH264DecoderDefault::copyFrame() {
    int w = mFrame->width;
    int h = mFrame->height;
    H264_DPRINT("w %d h %d Y line size %d U line size %d V line size %d", w, h,
            mFrame->linesize[0], mFrame->linesize[1], mFrame->linesize[2]);
    for (int i = 0; i < h; ++i) {
      memcpy(mDecodedFrame + i * w, mFrame->data[0] + i * mFrame->linesize[0], w);
    }
    for (int i=0; i < h / 2; ++i) {
      memcpy(w * h + mDecodedFrame + i * w/2, mFrame->data[1] + i * mFrame->linesize[1], w / 2);
    }
    for (int i=0; i < h / 2; ++i) {
      memcpy(w * h + w * h / 4 + mDecodedFrame + i * w/2, mFrame->data[2] + i * mFrame->linesize[2], w / 2);
    }
    mColorPrimaries = mFrame->color_primaries;
    mColorRange = mFrame->color_range;
    mColorTransfer = mFrame->color_trc;
    mColorSpace = mFrame->colorspace;
    H264_DPRINT("copied Frame and it has presentation time at %lld", (long long)(mFrame->pts));
    H264_DPRINT("Frame primary %d range %d transfer %d space %d", mFrame->color_primaries,
            mFrame->color_range, mFrame->color_trc, mFrame->colorspace);
}

void MediaH264DecoderDefault::flush(void* ptr) {
    H264_DPRINT("Flushing...");
    mIsInFlush = true;
    H264_DPRINT("Flushing done");
}

static uint8_t* getDst(void* ptr) {
    // Guest will pass us the offset from the start address for where to write the image data.
    uint8_t* xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t*)(xptr);
    return (uint8_t*)ptr + offset;
}

void MediaH264DecoderDefault::getImage(void* ptr) {
    H264_DPRINT("getImage %p", ptr);
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    int* retErr = (int*)(retptr);
    uint32_t* retWidth = (uint32_t*)(retptr + 8);
    uint32_t* retHeight = (uint32_t*)(retptr + 16);
    uint32_t* retPts = (uint32_t*)(retptr + 24);
    uint32_t* retColorPrimaries = (uint32_t*)(retptr + 32);
    uint32_t* retColorRange = (uint32_t*)(retptr + 40);
    uint32_t* retColorTransfer = (uint32_t*)(retptr + 48);
    uint32_t* retColorSpace = (uint32_t*)(retptr + 56);

    static int numbers=0;
    //H264_DPRINT("calling getImage %d", numbers++);
    if (!mDecodedFrame) {
        H264_DPRINT("%s: frame is null", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }
    if (!mImageReady) {
        if (mFrameFormatChanged) {
            *retWidth = mOutputWidth;
            *retHeight = mOutputHeight;
            *retErr = static_cast<int>(Err::DecoderRestarted);
            return;
        }
        if (mIsInFlush) {
            // guest be in flush mode, so try to get a frame
            avcodec_send_packet(mCodecCtx, NULL);
            int retframe = avcodec_receive_frame(mCodecCtx, mFrame);
            if (retframe == AVERROR(EAGAIN) || retframe == AVERROR_EOF) {
                H264_DPRINT("%s: frame is null", __func__);
                *retErr = static_cast<int>(Err::NoDecodedFrame);
                return;
            }

            H264_DPRINT("%s: got frame in flush mode retrun code %d", __func__, retframe);
            //now copy to mDecodedFrame
            copyFrame();
            mOutputPts = mFrame->pts;
            mImageReady = true;
        } else {
            H264_DPRINT("%s: no new frame yet", __func__);
            *retErr = static_cast<int>(Err::NoDecodedFrame);
            return;
        }
    }

    *retWidth = mOutputWidth;
    *retHeight = mOutputHeight;
    *retPts = mOutputPts;
    *retColorPrimaries = mColorPrimaries;
    *retColorRange = mColorRange;
    *retColorTransfer = mColorTransfer;
    *retColorSpace = mColorSpace;


    uint8_t* dst =  getDst(ptr);
    memcpy(dst, mDecodedFrame, mOutBufferSize);
    mImageReady = false;
    *retErr = mOutBufferSize;
}

}  // namespace emulation
}  // namespace android

