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
#include "android/emulation/MediaVpxDecoder.h"

#include <stdio.h>
#include <cassert>

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt,...) fprintf(stderr, "bohu-codec-vpx: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt,...)
#endif

#include <string.h>


namespace android {
namespace emulation {

void MediaVpxDecoder::handlePing(MediaCodecType type, MediaOperation op, void* ptr) {
    switch (op) {
        case MediaOperation::InitContext:
            VPX_DPRINT("init vpx");
            initVpxContext(ptr, type);
            break;
        case MediaOperation::DestroyContext:
            VPX_DPRINT("destroy context");
            destroyVpxContext(ptr);
            break;
        case MediaOperation::DecodeImage:
            VPX_DPRINT("decode");
            decodeFrame(ptr);
            break;
        case MediaOperation::Flush:
            VPX_DPRINT("flush");
            flush(ptr);
            break;
        case MediaOperation::GetImage:
            VPX_DPRINT("get img");
            getImage(ptr);
            break;
        default:
            VPX_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaVpxDecoder::save(base::Stream* stream) const {
    // NOT IMPLEMENTED
}

bool MediaVpxDecoder::load(base::Stream* stream) {
    // NOT IMPLEMENTED
    return true;
}

void MediaVpxDecoder::initVpxContext(void* ptr, MediaCodecType type) {
    assert(type == MediaCodecType::VP8Codec || type == MediaCodecType::VP9Codec);
    mCtx.reset(new vpx_codec_ctx_t);
    vpx_codec_err_t vpx_err;
    vpx_codec_dec_cfg_t cfg;
    vpx_codec_flags_t flags;
    memset(&cfg, 0, sizeof(cfg));
    memset(&flags, 0, sizeof(flags));
    cfg.threads = 4;

    if ((vpx_err = vpx_codec_dec_init(
                 mCtx.get(),
                 type == MediaCodecType::VP8Codec ?
                 &vpx_codec_vp8_dx_algo : &vpx_codec_vp9_dx_algo,
                 &cfg, flags))) {
        VPX_DPRINT("vpx decoder failed to initialize. (%d)", vpx_err);
    }
}

void MediaVpxDecoder::destroyVpxContext(void* ptr) {
    if (mCtx != nullptr) {
        vpx_codec_destroy(mCtx.get());
        mCtx.reset();
    }
}

static const uint8_t* getData(void* ptr) {
    uint64_t offset = *(uint64_t *)(ptr);
    return (const uint8_t*)ptr + offset;
}

static unsigned int getDataLen(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    unsigned int* pint = (unsigned int*)(xptr + 8);
    return *pint;
}

static void* getUserData(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t puser = *(uint64_t*)(xptr + 16);
    return (void*)puser;
}

void MediaVpxDecoder::decodeFrame(void* ptr) {
    const uint8_t * data = getData(ptr);
    unsigned int len = getDataLen(ptr);
    void* user_data = getUserData(ptr);

    VPX_DPRINT("bohu-codec-vpx: calling decodeFrame");
    vpx_codec_decode(mCtx.get(), data, len, user_data, 0);

    // now the we can call getImage
    mImageReady = true;
    mIter = NULL;
}

void MediaVpxDecoder::flush(void* ptr) {
    vpx_codec_decode(mCtx.get(), NULL, 0, NULL, 0);

    mImageReady = false;
    mIter = NULL;
}

void static copyYV12FrameToOutputBuffer(size_t outputBufferWidth, size_t outputBufferHeight,
        size_t imgWidth, size_t imgHeight, int32_t bpp,
        uint8_t *dst, const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV,
        size_t srcYStride, size_t srcUStride, size_t srcVStride) {

    size_t dstYStride = outputBufferWidth * bpp;
    size_t dstUVStride = dstYStride / 2;
    size_t dstHeight = outputBufferHeight;
    uint8_t *dstStart = dst;

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

static size_t getOutputBufferWidth(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    size_t* pint = (size_t *)(xptr + 0);
    return *pint;
}

static size_t getOutputBufferHeight(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    size_t* pint = (size_t *)(xptr + 8);
    return *pint;
}

static size_t getmWidth(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    size_t* pint = (size_t *)(xptr + 16);
    return *pint;
}

static size_t getmHeight(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    size_t* pint = (size_t *)(xptr + 24);
    return *pint;
}

static int32_t getbpp(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    int32_t * pint = (int32_t *)(xptr + 32);
    return *pint;
}

static uint8_t * getDst(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t *)(xptr + 40);
    return (uint8_t*)ptr + offset;
}

static int * getReturnAddress(void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    int * pint = (int *)(xptr + 256);
    return pint;
}

static void putImgFmt(vpx_img_fmt_t fmt, void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t * pint64 = (uint64_t *)(xptr + 256 + 8);
    *pint64 = fmt;
    VPX_DPRINT("put fmt %d", fmt);
}

static void putImgdw(unsigned int dw, void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t * pint64 = (uint64_t *)(xptr + 256 + 2 * 8);
    *pint64 = dw;
    VPX_DPRINT("put dw %d", dw);
}

static void putImgdh(unsigned int dh, void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t * pint64 = (uint64_t *)(xptr + 256 + 3 * 8);
    *pint64 = dh;
    VPX_DPRINT("put dh %d", dh);
}

static void putImgUserPriv(void* user_priv, void* ptr) {
    uint8_t * xptr = (uint8_t*)ptr;
    uint64_t * pint64 = (uint64_t *)(xptr + 256 + 4 * 8);
    *pint64 = (uint64_t)user_priv;
}

void static copyImgToGuest(vpx_image_t* mImg, void* ptr) {
    size_t outputBufferWidth = getOutputBufferWidth(ptr);
    size_t outputBufferHeight = getOutputBufferHeight(ptr);
    size_t mWidth = getmWidth(ptr);
    size_t mHeight = getmHeight(ptr);
    int32_t bpp = getbpp(ptr);
    uint8_t *dst = getDst(ptr);
    VPX_DPRINT("*** d_w=%d d_h=%d out_w=%d out_h=%d m_w=%d m_h=%d", mImg->d_w, mImg->d_h,
               (int)outputBufferWidth, (int)outputBufferHeight, (int)mWidth, (int)mHeight);

    const uint8_t *srcY = (const uint8_t *)mImg->planes[VPX_PLANE_Y];
    const uint8_t *srcU = (const uint8_t *)mImg->planes[VPX_PLANE_U];
    const uint8_t *srcV = (const uint8_t *)mImg->planes[VPX_PLANE_V];
    size_t srcYStride = mImg->stride[VPX_PLANE_Y];
    size_t srcUStride = mImg->stride[VPX_PLANE_U];
    size_t srcVStride = mImg->stride[VPX_PLANE_V];

    copyYV12FrameToOutputBuffer(outputBufferWidth, outputBufferHeight, mImg->d_w, mImg->d_h, bpp,
            dst, srcY, srcU, srcV, srcYStride, srcUStride, srcVStride);
}

void MediaVpxDecoder::getImage(void* ptr) {
    VPX_DPRINT("bohu-codec-vpx: calling getImage");
    if (!mImageReady) {
        VPX_DPRINT("bohu-codec-vpx: there is no image");
        return;
    }

    int * retptr = getReturnAddress(ptr);

    mImg = vpx_codec_get_frame(mCtx.get(), &mIter);
    if (mImg == NULL) {
        mImageReady = false;
        *retptr = 1;
        return;
    }

    copyImgToGuest(mImg, ptr);
    *retptr = 0;
    putImgFmt(mImg->fmt, ptr);
    putImgdw(mImg->d_w, ptr);
    putImgdh(mImg->d_h, ptr);
    putImgUserPriv(mImg->user_priv, ptr);
}

}  // namespace emulation
}  // namespace android
