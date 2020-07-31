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
#include "android/emulation/MediaVpxDecoderGeneric.h"
#include "android/base/system/System.h"
#include "android/emulation/MediaFfmpegVideoHelper.h"
#include "android/emulation/MediaVpxVideoHelper.h"
#include "android/emulation/VpxFrameParser.h"
#include "android/main-emugl.h"
#include "android/utils/debug.h"

#ifndef __APPLE__
// for Linux and Window, Cuvid is available
#include "android/emulation/MediaCudaDriverHelper.h"
#include "android/emulation/MediaCudaVideoHelper.h"
#endif

#include <stdio.h>
#include <cassert>
#include <functional>

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt, ...)                                                \
    fprintf(stderr, "media-vpx-decoder-generic: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt, ...)
#endif

#include <string.h>

namespace android {
namespace emulation {

using TextureFrame = MediaHostRenderer::TextureFrame;

namespace {

static bool s_cuvid_good = true;

static bool cudaVpxAllowed() {
    static std::once_flag once_flag;
    static bool s_cuda_vpx_allowed = false;

    std::call_once(once_flag, []() {
        {
            s_cuda_vpx_allowed =
                    android::base::System::getEnvironmentVariable(
                            "ANDROID_EMU_MEDIA_DECODER_CUDA_VPX") == "1";
        }
    });

    return s_cuda_vpx_allowed && s_cuvid_good;
}

bool canUseCudaDecoder() {
    // TODO: implement a whitelist for
    // nvidia gpu;
#ifndef __APPLE__
    if (cudaVpxAllowed() && MediaCudaDriverHelper::initCudaDrivers()) {
        VPX_DPRINT("Using Cuvid decoder on Linux/Windows");
        return true;
    } else {
        VPX_DPRINT(
                "ERROR: cannot use cuvid decoder: failed to init cuda driver");
        return false;
    }
#else
    return false;
#endif
}

bool canDecodeToGpuTexture() {
#ifndef __APPLE__

    if (cudaVpxAllowed() &&
        emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
        return true;
    } else {
        return false;
    }
#else
    return false;
#endif
}
};  // end namespace

MediaVpxDecoderGeneric::MediaVpxDecoderGeneric(VpxPingInfoParser parser,
                                               MediaCodecType type)
    : mType(type),
      mParser(parser),
      mSnapshotHelper(mType == MediaCodecType::VP8Codec
                              ? MediaSnapshotHelper::CodecType::VP8
                              : MediaSnapshotHelper::CodecType::VP9) {
    mUseGpuTexture = canDecodeToGpuTexture();
}

MediaVpxDecoderGeneric::~MediaVpxDecoderGeneric() {
    destroyVpxContext(nullptr);
}

void MediaVpxDecoderGeneric::initVpxContext(void* ptr) {
    VPX_DPRINT("calling init context");

#ifndef __APPLE__
    if (canUseCudaDecoder() && mParser.version() >= 200) {
        MediaCudaVideoHelper::OutputTreatmentMode oMode =
                MediaCudaVideoHelper::OutputTreatmentMode::SAVE_RESULT;

        MediaCudaVideoHelper::FrameStorageMode fMode =
                mUseGpuTexture ? MediaCudaVideoHelper::FrameStorageMode::
                                         USE_GPU_TEXTURE
                               : MediaCudaVideoHelper::FrameStorageMode::
                                         USE_BYTE_BUFFER;

        auto cudavid = new MediaCudaVideoHelper(
                oMode, fMode,
                mType == MediaCodecType::VP8Codec ? cudaVideoCodec_VP8
                                                  : cudaVideoCodec_VP9);

        if (mUseGpuTexture) {
            cudavid->resetTexturePool(mRenderer.getTexturePool());
        }
        mHwVideoHelper.reset(cudavid);
        if (!mHwVideoHelper->init()) {
            mHwVideoHelper.reset(nullptr);
        }
    }
#endif

    if (mHwVideoHelper == nullptr) {
        createAndInitSoftVideoHelper();
    }

    VPX_DPRINT("vpx decoder initialize context successfully.");
}

void MediaVpxDecoderGeneric::createAndInitSoftVideoHelper() {
    if (false && mParser.version() == 200 &&
        (mType == MediaCodecType::VP8Codec)) {
        // disable ffmpeg vp8 for now, until further testing
        // vp8 and render to host
        mSwVideoHelper.reset(new MediaFfmpegVideoHelper(
                mType == MediaCodecType::VP8Codec ? 8 : 9,
                mParser.version() < 200 ? 1 : 4));
    } else {
        mSwVideoHelper.reset(new MediaVpxVideoHelper(
                mType == MediaCodecType::VP8Codec ? 8 : 9,
                mParser.version() < 200 ? 1 : 4));
    }
    mSwVideoHelper->init();
}

void MediaVpxDecoderGeneric::decodeFrame(void* ptr) {
    VPX_DPRINT("calling decodeFrame");
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* data = param.p_data;
    unsigned int len = param.size;

    mSnapshotHelper.savePacket(data, len, param.user_priv);
    VPX_DPRINT("calling vpx_codec_decode data %p datalen %d userdata %" PRIx64,
               data, (int)len, param.user_priv);

    decode_internal(data, len, param.user_priv);

    // now the we can call getImage
    fetchAllFrames();
    ++mNumFramesDecoded;
    VPX_DPRINT("decoded %d frames", mNumFramesDecoded);
}

void MediaVpxDecoderGeneric::decode_internal(const uint8_t* data,
                                             size_t len,
                                             uint64_t pts) {
    if (mTrialPeriod) {
        try_decode(data, len, pts);
    } else {
        mVideoHelper->decode(data, len, pts);
    }
}

void MediaVpxDecoderGeneric::try_decode(const uint8_t* data,
                                        size_t len,
                                        uint64_t pts) {
    // for vpx, the first frame is enough to decide
    // whether hw decoder can handle it
    // probably need a whitelist for nvidia gpu
    if (mHwVideoHelper != nullptr) {
        mHwVideoHelper->decode(data, len, pts);
        if (mHwVideoHelper->good()) {
            mVideoHelper = std::move(mHwVideoHelper);
            mTrialPeriod = false;
            return;
        } else {
            VPX_DPRINT("Switching from HW to SW codec");
            dprint("Failed to decode with HW decoder (Error Code: %d); switch "
                   "to SW",
                   mHwVideoHelper->error());
            mUseGpuTexture = false;
            if (mHwVideoHelper->fatal()) {
                s_cuvid_good = false;
            }
            mHwVideoHelper.reset(nullptr);
        }
    }
    // just use libvpx, it is better quality in general
    // except not so parallel on vp8
    mSwVideoHelper.reset(
            new MediaVpxVideoHelper(mType == MediaCodecType::VP8Codec ? 8 : 9,
                                    mParser.version() < 200 ? 1 : 4));

    mSwVideoHelper->init();
    mVideoHelper = std::move(mSwVideoHelper);
    mVideoHelper->decode(data, len, pts);
    mTrialPeriod = false;
}

void MediaVpxDecoderGeneric::fetchAllFrames() {
    while (true) {
        MediaSnapshotState::FrameInfo frame;
        bool success = mVideoHelper->receiveFrame(&frame);
        if (!success) {
            break;
        }
        mSnapshotHelper.saveDecodedFrame(std::move(frame));
    }
}

void MediaVpxDecoderGeneric::getImage(void* ptr) {
    VPX_DPRINT("calling getImage");
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retptr = param.p_error;
    MediaSnapshotState::FrameInfo* pFrame = mSnapshotHelper.frontFrame();
    if (pFrame == nullptr) {
        VPX_DPRINT("there is no image");
        *retptr = 1;
        return;
    }

    *retptr = 0;
    *(param.p_fmt) = VPX_IMG_FMT_I420;
    *(param.p_d_w) = pFrame->width;
    *(param.p_d_h) = pFrame->height;
    *(param.p_user_priv) = pFrame->pts;
    VPX_DPRINT("got time %" PRIx64, pFrame->pts);
    VPX_DPRINT(
            "fmt is %d  I42016 is %d I420 is %d userdata is %p colorbuffer id "
            "%d bpp %d",
            (int)(*param.p_fmt), (int)VPX_IMG_FMT_I42016, (int)VPX_IMG_FMT_I420,
            (void*)(*(param.p_user_priv)), param.hostColorBufferId,
            (int)param.bpp);

    if (mParser.version() == 200) {
        VPX_DPRINT("calling rendering to host side color buffer with id %d",
                   param.hostColorBufferId);
        if (mUseGpuTexture && pFrame->texture[0] > 0 && pFrame->texture[1] > 0) {
            VPX_DPRINT(
                    "calling rendering to host side color buffer with id %d "
                    "(gpu texture mode: textures %u %u)",
                    param.hostColorBufferId,
                    pFrame->texture[0],
                    pFrame->texture[1]);
            mRenderer.renderToHostColorBufferWithTextures(
                    param.hostColorBufferId, pFrame->width, pFrame->height,
                    TextureFrame{pFrame->texture[0], pFrame->texture[1]});
        } else {
            VPX_DPRINT(
                    "calling rendering to host side color buffer with id %d",
                    param.hostColorBufferId);
            mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                              pFrame->width, pFrame->height,
                                              pFrame->data.data());
        }
    } else {
        memcpy(param.p_dst, pFrame->data.data(),
               pFrame->width * pFrame->height * 3 / 2);
    }
    mSnapshotHelper.discardFrontFrame();
    VPX_DPRINT("completed getImage with colorid %d",
               (int)param.hostColorBufferId);
}

void MediaVpxDecoderGeneric::flush(void* ptr) {
    VPX_DPRINT("calling flush");
    if (mVideoHelper) {
        mVideoHelper->flush();
        fetchAllFrames();
    }
    VPX_DPRINT("flush done");
}

void MediaVpxDecoderGeneric::destroyVpxContext(void* ptr) {
    VPX_DPRINT("calling destroy context");
    if (mVideoHelper != nullptr) {
        mVideoHelper->deInit();
        mVideoHelper.reset(nullptr);
    }
}

void MediaVpxDecoderGeneric::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mVideoHelper != nullptr ? 1 : 0);

    mSnapshotHelper.save(stream);
}

void MediaVpxDecoderGeneric::oneShotDecode(const uint8_t* data,
                                           size_t len,
                                           uint64_t pts) {
    if (!mHwVideoHelper && !mSwVideoHelper && !mVideoHelper) {
        return;
    }

    decode_internal(data, len, pts);
    mVideoHelper->decode(data, len, pts);
    while (true) {
        MediaSnapshotState::FrameInfo frame;
        bool success = mVideoHelper->receiveFrame(&frame);
        if (!success) {
            break;
        }
        }
}

bool MediaVpxDecoderGeneric::load(base::Stream* stream) {
    VPX_DPRINT("loading libvpx now type %d",
               mType == MediaCodecType::VP8Codec ? 8 : 9);
    uint32_t version = stream->getBe32();
    mParser = VpxPingInfoParser{version};

    int savedState = stream->getBe32();
    if (savedState == 1) {
        initVpxContext(nullptr);
    }

    std::function<void(const uint8_t*, size_t, uint64_t)> func =
            [=](const uint8_t* data, size_t len, uint64_t pts) {
                this->oneShotDecode(data, len, pts);
            };

    if (mVideoHelper) {
        mVideoHelper->setIgnoreDecodedFrames();
    }

    mSnapshotHelper.load(stream, func);

    if (mVideoHelper) {
        mVideoHelper->setSaveDecodedFrames();
    }

    VPX_DPRINT("Done loading snapshots frames\n\n");
    return true;
}

}  // namespace emulation
}  // namespace android
