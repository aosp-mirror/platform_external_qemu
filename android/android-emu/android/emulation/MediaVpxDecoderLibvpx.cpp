// Copyright 2019 The Android Open Source Project
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
#include "android/emulation/MediaVpxDecoderLibvpx.h"

#include <stdio.h>
#include <cassert>

#define MEDIA_VPX_DEBUG 1

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt, ...)                                               \
    fprintf(stderr, "media-vp9-decoder-libvpx: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt, ...)
#endif

#include <string.h>

namespace android {
namespace emulation {

void MediaVpxDecoderLibvpx::initVpxContext(void* ptr) {
    VPX_DPRINT("calling init context");
    mCtx.reset(new vpx_codec_ctx_t);
    vpx_codec_err_t vpx_err;
    vpx_codec_dec_cfg_t cfg;
    vpx_codec_flags_t flags;
    memset(&cfg, 0, sizeof(cfg));
    memset(&flags, 0, sizeof(flags));
    cfg.threads = 4;

    if ((vpx_err = vpx_codec_dec_init(mCtx.get(),
                                      mType == MediaCodecType::VP8Codec
                                              ? &vpx_codec_vp8_dx_algo
                                              : &vpx_codec_vp9_dx_algo,
                                      &cfg, flags))) {
        VPX_DPRINT("vpx decoder failed to initialize. (%d)", vpx_err);
        mCtx.reset();
    }
}

void MediaVpxDecoderLibvpx::decodeFrame(void* ptr) {
    VPX_DPRINT("calling decodeFrame");
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* data = param.p_data;
    unsigned int len = param.size;
    void* user_data = (void*)(param.user_priv);

    VPX_DPRINT("calling vpx_codec_decode data %p datalen %d userdata %p", data,
               (int)len, user_data);

    vpx_codec_decode(mCtx.get(), data, len, user_data, 0);

    // now the we can call getImage
    mImageReady = true;
    mIter = NULL;
}

void MediaVpxDecoderLibvpx::getImage(void* ptr) {
    VPX_DPRINT("calling getImage");
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    if (!mImageReady) {
        // VPX_DPRINT("there is no image");
        return;
    }

    int* retptr = param.p_error;

    mImg = vpx_codec_get_frame(mCtx.get(), &mIter);
    if (mImg == NULL) {
        mImageReady = false;
        // VPX_DPRINT("there is no image");
        *retptr = 1;
        return;
    }

    copyImgToGuest(mImg, param);
    *retptr = 0;
    *(param.p_fmt) = mImg->fmt;
    *(param.p_d_w) = mImg->d_w;
    *(param.p_d_h) = mImg->d_h;
    *(param.p_user_priv) = (uint64_t)(mImg->user_priv);
    //(mImg->fmt == VPX_IMG_FMT_I420 || mImg->fmt == VPX_IMG_FMT_I42016)
    VPX_DPRINT(
            "fmt is %d  I42016 is %d I420 is %d userdata is %p colorbuffer id "
            "%d bpp %d",
            (int)(*param.p_fmt), (int)VPX_IMG_FMT_I42016, (int)VPX_IMG_FMT_I420,
            (void*)(*(param.p_user_priv)), param.hostColorBufferId,
            (int)param.bpp);

    if (mParser.version() == 200) {
        if (param.hostColorBufferId >= 0) {
            VPX_DPRINT("calling rendering to host side color buffer with id %d",
                       param.hostColorBufferId);
            mRenderer.renderToHostColorBuffer(
                    param.hostColorBufferId, param.outputBufferWidth,
                    param.outputBufferHeight, param.p_dst);
        }
    }
}

void MediaVpxDecoderLibvpx::flush(void* ptr) {
    VPX_DPRINT("calling flush");
    vpx_codec_decode(mCtx.get(), NULL, 0, NULL, 0);

    mImageReady = false;
    mIter = NULL;
}

void MediaVpxDecoderLibvpx::destroyVpxContext(void* ptr) {
    if (mCtx != nullptr) {
        VPX_DPRINT("calling destroy context");
        vpx_codec_destroy(mCtx.get());
        mCtx.reset();
    }
}

void MediaVpxDecoderLibvpx::copyYV12FrameToOutputBuffer(
        size_t outputBufferWidth,
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

void MediaVpxDecoderLibvpx::copyImgToGuest(vpx_image_t* mImg,
                                           const GetImageParam& param) {
    size_t outputBufferWidth = param.outputBufferWidth;
    size_t outputBufferHeight = param.outputBufferHeight;
    size_t mWidth = param.width;
    size_t mHeight = param.height;
    int32_t bpp = param.bpp;
    uint8_t* dst = param.p_dst;
    VPX_DPRINT("*** d_w=%d d_h=%d out_w=%d out_h=%d m_w=%d m_h=%d bpp=%d",
               mImg->d_w, mImg->d_h, (int)outputBufferWidth,
               (int)outputBufferHeight, (int)mWidth, (int)mHeight, (int)bpp);

    const uint8_t* srcY = (const uint8_t*)mImg->planes[VPX_PLANE_Y];
    const uint8_t* srcU = (const uint8_t*)mImg->planes[VPX_PLANE_U];
    const uint8_t* srcV = (const uint8_t*)mImg->planes[VPX_PLANE_V];
    size_t srcYStride = mImg->stride[VPX_PLANE_Y];
    size_t srcUStride = mImg->stride[VPX_PLANE_U];
    size_t srcVStride = mImg->stride[VPX_PLANE_V];

    copyYV12FrameToOutputBuffer(outputBufferWidth, outputBufferHeight,
                                mImg->d_w, mImg->d_h, bpp, dst, srcY, srcU,
                                srcV, srcYStride, srcUStride, srcVStride);
}

MediaVpxDecoderLibvpx::MediaVpxDecoderLibvpx(VpxPingInfoParser parser,
                                             MediaCodecType type)
    : mType(type), mParser(parser) {}

MediaVpxDecoderLibvpx::~MediaVpxDecoderLibvpx() {
    destroyVpxContext(nullptr);
}

}  // namespace emulation
}  // namespace android
