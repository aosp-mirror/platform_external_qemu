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

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt,...)
#endif

#include <string.h>


namespace android {
namespace emulation {

enum vpx_metadata {
    VPX_INIT_CONTEXT = 1,
    VPX_DESTROY_CONTEXT = 2,
    VPX_DECODE = 3,
    VPX_GET_IMGE = 4,
};

void MediaVpxDecoder::handlePing(uint64_t metadata, void* ptr) {
    switch (metadata) {
        case VPX_INIT_CONTEXT:
            initVpxContext(ptr);
            break;
        case VPX_DESTROY_CONTEXT:
            destroyVpxContext(ptr);
            break;
        case VPX_DECODE:
            decodeFrame(ptr);
            break;
        case VPX_GET_IMGE:
            getImage(ptr);
            break;
        default:
            VPX_DPRINT("Unknown command %llu\n", metadata);
            break;
    }
}

void MediaVpxDecoder::initVpxContext(void* ptr) {
    mCtx = new vpx_codec_ctx_t;
    vpx_codec_err_t vpx_err;
    vpx_codec_dec_cfg_t cfg;
    vpx_codec_flags_t flags;
    memset(&cfg, 0, sizeof(vpx_codec_dec_cfg_t));
    memset(&flags, 0, sizeof(vpx_codec_flags_t));
    cfg.threads = 4;

    if ((vpx_err = vpx_codec_dec_init(
                 mCtx,
                 &vpx_codec_vp9_dx_algo,
                 &cfg, flags))) {
        VPX_DPRINT("vpx decoder failed to initialize. (%d)", vpx_err);
    }    
}

void MediaVpxDecoder::destroyVpxContext(void* ptr) {
    vpx_codec_destroy(mCtx);
    delete mCtx;
    mCtx = nullptr;
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

/*
 vpx_codec_decode(vpx_codec_ctx_t *ctx, const uint8_t *data,
                                 unsigned int data_sz, void *user_priv,
                                 long deadline);
   */
void MediaVpxDecoder::decodeFrame(void* ptr) {
    const uint8_t * data = getData(ptr);
    unsigned int len = getDataLen(ptr);
    void* user_data = getUserData(ptr);

    vpx_codec_decode(mCtx, data, len, user_data, 0);

    // now the we can call getImage
    mImageReady = true;
    mIter = NULL;
}

void MediaVpxDecoder::getImage(void* ptr) {
    if (!mImageReady) {
        VPX_DPRINT("there is no image");
        return;
    }

    mImg = vpx_codec_get_frame(mCtx, &mIter);
    if (mImg == NULL) {
        mImageReady = false;
    }
}

}  // namespace emulation
}  // namespace android
