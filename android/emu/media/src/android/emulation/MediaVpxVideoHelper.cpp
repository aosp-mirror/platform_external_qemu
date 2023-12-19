// Copyright (C) 2020 The Android Open Source Project
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

#include "host-common/MediaVpxVideoHelper.h"
#include "host-common/YuvConverter.h"
#include "android/utils/debug.h"

#include <cassert>

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define MEDIA_DPRINT(fmt, ...)                                           \
    fprintf(stderr, "media-vpx-video-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define MEDIA_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using FrameInfo = MediaSnapshotState::FrameInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;

MediaVpxVideoHelper::MediaVpxVideoHelper(int type, int threads)
    : mType(type), mThreadCount(threads) {}

MediaVpxVideoHelper::~MediaVpxVideoHelper() {
    deInit();
}

void MediaVpxVideoHelper::decode(const uint8_t* data,
                                 size_t len,
                                 uint64_t pts) {
    MEDIA_DPRINT("decoding %d bytes", (int)len);
    vpx_codec_decode(mCtx.get(), data, len, (void*)pts, 0);
    fetchAllFrames();
}

void MediaVpxVideoHelper::flush() {
    MEDIA_DPRINT("calling flush");
    vpx_codec_decode(mCtx.get(), NULL, 0, NULL, 0);
    fetchAllFrames();
}

bool MediaVpxVideoHelper::init() {
    MEDIA_DPRINT("calling init context");
    mCtx.reset(new vpx_codec_ctx_t);
    vpx_codec_err_t vpx_err;
    vpx_codec_dec_cfg_t cfg;
    vpx_codec_flags_t flags;
    memset(&cfg, 0, sizeof(cfg));
    memset(&flags, 0, sizeof(flags));
    cfg.threads = 1;

    if (mThreadCount > 1) {
        cfg.threads = std::min(mThreadCount, 4);
    }

    if ((vpx_err = vpx_codec_dec_init(
                 mCtx.get(),
                 mType == 8 ? &vpx_codec_vp8_dx_algo : &vpx_codec_vp9_dx_algo,
                 &cfg, flags))) {
        MEDIA_DPRINT("vpx decoder failed to initialize. (%d)", vpx_err);
        mCtx.reset();
        return false;
    }
    MEDIA_DPRINT("vpx decoder initialize context successfully.");
    dprint("successfully created libvpx video decoder for VP%d", mType);

    return true;
}

void MediaVpxVideoHelper::deInit() {
    if (mCtx != nullptr) {
        MEDIA_DPRINT("calling destroy context");
        vpx_codec_destroy(mCtx.get());
        mCtx.reset();
    }
}

void MediaVpxVideoHelper::copyYV12FrameToOutputBuffer(size_t outputBufferWidth,
                                                      size_t outputBufferHeight,
                                                      size_t imgWidth,
                                                      size_t imgHeight,
                                                      int32_t bpp,
                                                      uint8_t* dst,
                                                      const uint8_t* srcY,
                                                      const uint8_t* srcU,
                                                      const uint8_t* srcV,
                                                      size_t srcYStride,
                                                      size_t srcUStride,
                                                      size_t srcVStride) {
    size_t dstYStride = outputBufferWidth * bpp;
    size_t dstUVStride = dstYStride / 2;
    size_t dstHeight = outputBufferHeight;
    uint8_t* dstStart = dst;

    for (size_t i = 0; i < imgHeight; ++i) {
        memcpy(dst, srcY, imgWidth * bpp);
        srcY += srcYStride;
        dst += dstYStride;
    }

    dst = dstStart + dstYStride * dstHeight;
    for (size_t i = 0; i < imgHeight / 2; ++i) {
        memcpy(dst, srcU, imgWidth / 2 * bpp);
        srcU += srcUStride;
        dst += dstUVStride;
    }

    dst = dstStart + (5 * dstYStride * dstHeight) / 4;
    for (size_t i = 0; i < imgHeight / 2; ++i) {
        memcpy(dst, srcV, imgWidth / 2 * bpp);
        srcV += srcVStride;
        dst += dstUVStride;
    }
}

static void convertYUV420Planar16ToYUV420Planar(
        uint8_t *dstY, uint8_t *dstU, uint8_t *dstV,
        const uint16_t *srcY, const uint16_t *srcU, const uint16_t *srcV,
        size_t srcYStride, size_t srcUStride, size_t srcVStride,
        size_t dstYStride, size_t dstUVStride,
        size_t width, size_t height) {

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            dstY[x] = (uint8_t)(srcY[x] >> 2);
        }

        srcY += srcYStride;
        dstY += dstYStride;
    }

    for (size_t y = 0; y < (height + 1) / 2; ++y) {
        for (size_t x = 0; x < (width + 1) / 2; ++x) {
            dstU[x] = (uint8_t)(srcU[x] >> 2);
            dstV[x] = (uint8_t)(srcV[x] >> 2);
        }

        srcU += srcUStride;
        srcV += srcVStride;
        dstU += dstUVStride;
        dstV += dstUVStride;
    }
    return;
}

void MediaVpxVideoHelper::copyImgToGuest(vpx_image_t* mImg,
                                         std::vector<uint8_t>& byteBuffer) {
    size_t outputBufferWidth = mImg->d_w;
    size_t outputBufferHeight = mImg->d_h;
    size_t mWidth = mImg->d_w;
    size_t mHeight = mImg->d_h;
    assert(mImg->fmt == VPX_IMG_FMT_I420 || mImg->fmt == VPX_IMG_FMT_I42016);
    int32_t bpp = (mImg->fmt == VPX_IMG_FMT_I420) ? 1 : 2;

    byteBuffer.resize(mWidth * mHeight * bpp * 3 / 2);
    uint8_t* dst = byteBuffer.data();
    MEDIA_DPRINT("*** d_w=%d d_h=%d out_w=%d out_h=%d m_w=%d m_h=%d bpp=%d",
                 mImg->d_w, mImg->d_h, (int)outputBufferWidth,
                 (int)outputBufferHeight, (int)mWidth, (int)mHeight, (int)bpp);

    const uint8_t* srcY = (const uint8_t*)mImg->planes[VPX_PLANE_Y];
    const uint8_t* srcU = (const uint8_t*)mImg->planes[VPX_PLANE_U];
    const uint8_t* srcV = (const uint8_t*)mImg->planes[VPX_PLANE_V];
    size_t srcYStride = mImg->stride[VPX_PLANE_Y];
    size_t srcUStride = mImg->stride[VPX_PLANE_U];
    size_t srcVStride = mImg->stride[VPX_PLANE_V];

    if (bpp == 2) {
        size_t dstYStride = outputBufferWidth;
        size_t dstUVStride = dstYStride / 2;
        uint8_t *dstY = dst;
        uint8_t *dstU = dst + dstYStride * outputBufferHeight;
        uint8_t *dstV = dst + (5 * dstYStride * outputBufferHeight) / 4;
        convertYUV420Planar16ToYUV420Planar(dstY, dstU, dstV,
                (const uint16_t*)(srcY), (const uint16_t*)(srcU), (const uint16_t*)(srcV),
                srcYStride>>1, srcUStride>>1, srcVStride>>1,
                dstYStride, dstUVStride,
                outputBufferWidth, outputBufferHeight
                );

    } else {
        copyYV12FrameToOutputBuffer(outputBufferWidth, outputBufferHeight,
                                mImg->d_w, mImg->d_h, bpp, dst, srcY, srcU,
                                srcV, srcYStride, srcUStride, srcVStride);
    }
}

void MediaVpxVideoHelper::fetchAllFrames() {
    mIter = NULL;
    while (true) {
        mImg = vpx_codec_get_frame(mCtx.get(), &mIter);
        if (mImg == NULL) {
            break;
        }
        if (mIgnoreDecoderOutput) {
            continue;
        }
        std::vector<uint8_t> byteBuffer;
        copyImgToGuest(mImg, byteBuffer);
        MEDIA_DPRINT("save frame");

        //  include/vpx/vpx_image.h
        unsigned int color_primaries = 2;  // default unspecified
        if (mImg->cs == VPX_CS_BT_601) {
            color_primaries = 4;
        } else if (mImg->cs == VPX_CS_BT_709) {
            color_primaries = 1;
        }
        // Note: the color primary and range from bitstream seems not trustable
        color_primaries = 2;  // default unspecified

        MEDIA_DPRINT("vpx fetch primary as %d orgcs %d\n", (int)color_primaries,
                     mImg->cs);

        unsigned int color_range = 0;  // default unspecified
        if (mImg->range == VPX_CR_FULL_RANGE) {
            color_range = 1;  // full range
        } else if (mImg->range == VPX_CR_STUDIO_RANGE) {
            color_range = 2;  // limited range
        }
        color_range = 0;  // default unspecified
        MEDIA_DPRINT("vpx fetch color range as %d orgrange %d\n",
                     (int)color_range, mImg->range);
        const unsigned int color_trc = 3;   // default 3; don't care for now
        const unsigned int colorspace = 2;  // unspecified, dont care for now

        mSavedDecodedFrames.push_back(MediaSnapshotState::FrameInfo{
                std::move(byteBuffer), std::vector<uint32_t>{}, (int)mImg->d_w,
                (int)mImg->d_h, (uint64_t)(mImg->user_priv),
                ColorAspects{color_primaries, color_range, color_trc,
                             colorspace}});
    }
}

}  // namespace emulation
}  // namespace android
