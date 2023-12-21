// Copyright (C) 2022 The Android Open Source Project
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

#include "android/emulation/MediaHevcDecoderGeneric.h"
#include "android/base/system/System.h"
#include "android/emulation/HevcPingInfoParser.h"
#include "host-common/MediaFfmpegVideoHelper.h"
#include "host-common/opengl/emugl_config.h"

#ifndef __APPLE__
// for Linux and Window, Cuvid is available
#include "host-common/MediaCudaDriverHelper.h"
#include "host-common/MediaCudaVideoHelper.h"
#else
#include "android/emulation/MediaVideoToolBoxHevcVideoHelper.h"
#endif

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_HEVC_DEBUG 0

#if MEDIA_HEVC_DEBUG
#define HEVC_DPRINT(fmt, ...)                                                \
    fprintf(stderr, "hevc-dec-generic: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define HEVC_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using InitContextParam = HevcPingInfoParser::InitContextParam;
using DecodeFrameParam = HevcPingInfoParser::DecodeFrameParam;
using ResetParam = HevcPingInfoParser::ResetParam;
using GetImageParam = HevcPingInfoParser::GetImageParam;
using TextureFrame = MediaHostRenderer::TextureFrame;

namespace {

bool canUseCudaDecoder() {
#ifndef __APPLE__
    // enable cuda by default, unless it is explicitly disallowed
    const bool is_cuda_disabled =
            (android::base::System::getEnvironmentVariable(
                     "ANDROID_EMU_MEDIA_DECODER_CUDA_HEVC") == "0");
    if (is_cuda_disabled)
        return false;

    if (MediaCudaDriverHelper::initCudaDrivers()) {
        HEVC_DPRINT("Using Cuvid decoder on Linux/Windows");
        return true;
    } else {
        HEVC_DPRINT(
                "ERROR: cannot use cuvid decoder: failed to init cuda driver");
        return false;
    }
#else
    return false;
#endif
}

bool canDecodeToGpuTexture(int w, int h) {
    if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
        int64_t ww = w;
        int64_t hh = h;
        constexpr int ONE_MILLION_PIXELS = 1000 * 1000;
        if (ww * hh <= ONE_MILLION_PIXELS) {
            return false;
        }
        return true;
    } else {
        return false;
    }
}
};  // end namespace

MediaHevcDecoderGeneric::MediaHevcDecoderGeneric(uint64_t id,
                                                 HevcPingInfoParser parser)
    : mId(id), mParser(parser) {
    HEVC_DPRINT("allocated MediaHevcDecoderGeneric %p with version %d", this,
                (int)mParser.version());
}

MediaHevcDecoderGeneric::~MediaHevcDecoderGeneric() {
    HEVC_DPRINT("destroyed MediaHevcDecoderGeneric %p", this);
    destroyHevcContext();
}

void MediaHevcDecoderGeneric::reset(void* ptr) {
    destroyHevcContext();
    ResetParam param{};
    mParser.parseResetParams(ptr, param);
    initHevcContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaHevcDecoderGeneric::initHevcContext(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initHevcContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaHevcDecoderGeneric::initHevcContextInternal(unsigned int width,
                                                      unsigned int height,
                                                      unsigned int outWidth,
                                                      unsigned int outHeight,
                                                      PixelFormat outPixFmt) {
    HEVC_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)", __func__, width,
                height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;

    mUseGpuTexture = canDecodeToGpuTexture(width, height);

#ifdef __APPLE__
    // enable vtb by default, unless it is explicitly disallowed (for test
    // purpose)
    bool is_vtb_allowed = !(android::base::System::getEnvironmentVariable(
                                    "ANDROID_EMU_MEDIA_DECODER_VTB") == "0");

    if (is_vtb_allowed) {
        if (width < 256 || height < 256) {
            // when it is small size, just copy memory
            // instead of texture transfer which has accuracy
            // issue with small sizes b/191768035
            mUseGpuTexture = false;
            HEVC_DPRINT("OSX: gpu texture mode is turned off for w %" PRIu32
                        " h %" PRIu32,
                        width, height);
        }
#ifdef __x86_64__
        // b/210512774
        // texture copy breaks on Intel Monterey
        mUseGpuTexture = false;
#endif
        MediaVideoToolBoxHevcVideoHelper::FrameStorageMode fMode =
                (mParser.version() >= 200 && mUseGpuTexture)
                        ? MediaVideoToolBoxHevcVideoHelper::FrameStorageMode::
                                  USE_GPU_TEXTURE
                        : MediaVideoToolBoxHevcVideoHelper::FrameStorageMode::
                                  USE_BYTE_BUFFER;
        auto macDecoder = new MediaVideoToolBoxHevcVideoHelper(
                mOutputWidth, mOutputHeight,
                MediaVideoToolBoxHevcVideoHelper::OutputTreatmentMode::
                        SAVE_RESULT,
                fMode);

        if (mUseGpuTexture && mParser.version() >= 200) {
            HEVC_DPRINT("use gpu texture on OSX");
            macDecoder->resetTexturePool(mRenderer.getTexturePool());
        }
        mHwVideoHelper.reset(macDecoder);
        mHwVideoHelper->init();
    }
#else
    bool sizeBigEnough = true;
    if (width <= 128 || height <= 128) {
        // when it is small size, just fall back to ffmpeg
        mUseGpuTexture = false;
        sizeBigEnough = false;
        HEVC_DPRINT("nvidia: decoder is turned off for w %" PRIu32
                    " h %" PRIu32,
                    width, height);
    }

    if (sizeBigEnough && canUseCudaDecoder()) {
        MediaCudaVideoHelper::OutputTreatmentMode oMode =
                MediaCudaVideoHelper::OutputTreatmentMode::SAVE_RESULT;

        if (mParser.version() < 200) {
            // when decoding into bytebuffer, do not use gpu texture mode
            mUseGpuTexture = false;
        }

        MediaCudaVideoHelper::FrameStorageMode fMode =
                mUseGpuTexture ? MediaCudaVideoHelper::FrameStorageMode::
                                         USE_GPU_TEXTURE
                               : MediaCudaVideoHelper::FrameStorageMode::
                                         USE_BYTE_BUFFER;

        auto cudavid =
                new MediaCudaVideoHelper(oMode, fMode, cudaVideoCodec_HEVC);

        if (mUseGpuTexture) {
            HEVC_DPRINT("use gpu texture");
            cudavid->resetTexturePool(mRenderer.getTexturePool());
        }
        mHwVideoHelper.reset(cudavid);
        if (!mHwVideoHelper->init()) {
            mHwVideoHelper.reset(nullptr);
            HEVC_DPRINT("failed to init cuda decoder");
        } else {
            HEVC_DPRINT("succeeded to init cuda decoder");
        }
    } else {
        mTrialPeriod = false;
        createAndInitSoftVideoHelper();
        mVideoHelper = std::move(mSwVideoHelper);
    }
#endif

    mSnapshotHelper.reset(
            new MediaSnapshotHelper(MediaSnapshotHelper::CodecType::HEVC));

    HEVC_DPRINT("Successfully created hevc decoder context %p", this);
}

void MediaHevcDecoderGeneric::createAndInitSoftVideoHelper() {
    mSwVideoHelper.reset(
            new MediaFfmpegVideoHelper(265, mParser.version() < 200 ? 1 : 4));
    mUseGpuTexture = false;
    mSwVideoHelper->init();
}

MediaHevcDecoderPlugin* MediaHevcDecoderGeneric::clone() {
    HEVC_DPRINT("clone MediaHevcDecoderGeneric %p with version %d", this,
                (int)mParser.version());
    return new MediaHevcDecoderGeneric(mId, mParser);
}

void MediaHevcDecoderGeneric::destroyHevcContext() {
    HEVC_DPRINT("Destroy context %p", this);
    while (mSnapshotHelper != nullptr) {
        MediaSnapshotState::FrameInfo* pFrame = mSnapshotHelper->frontFrame();
        if (!pFrame) {
            break;
        }
        if (mUseGpuTexture && pFrame->texture[0] > 0 &&
            pFrame->texture[1] > 0) {
            mRenderer.putTextureFrame(
                    TextureFrame{pFrame->texture[0], pFrame->texture[1]});
        }
        mSnapshotHelper->discardFrontFrame();
    }
    mRenderer.cleanUpTextures();

    if (mHwVideoHelper != nullptr) {
        mHwVideoHelper->deInit();
        mHwVideoHelper.reset(nullptr);
    }
    if (mVideoHelper != nullptr) {
        mVideoHelper->deInit();
        mVideoHelper.reset(nullptr);
    }
}

void MediaHevcDecoderGeneric::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);
    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t inputPts = param.pts;

    HEVC_DPRINT("%s(frame=%p, sz=%zu pts %lld)", __func__, frame, szBytes,
                (long long)inputPts);
    size_t* retSzBytes = param.pConsumedBytes;
    int32_t* retErr = param.pDecoderErrorCode;

    decodeFrameInternal(frame, szBytes, inputPts);

    mSnapshotHelper->savePacket(frame, szBytes, inputPts);
    fetchAllFrames();

    *retSzBytes = szBytes;
    *retErr = (int32_t)Err::NoErr;

    HEVC_DPRINT("done decoding this frame");
}

void MediaHevcDecoderGeneric::decodeFrameInternal(const uint8_t* data,
                                                  size_t len,
                                                  uint64_t pts) {
    if (mTrialPeriod) {
        HEVC_DPRINT("still in trial period");
        try_decode(data, len, pts);
    } else {
        mVideoHelper->decode(data, len, pts);
    }
}

void MediaHevcDecoderGeneric::try_decode(const uint8_t* data,
                                         size_t len,
                                         uint64_t pts) {
    // for hevc, it needs sps/pps to decide whether decoding is
    // possible;
    if (mHwVideoHelper != nullptr) {
        mHwVideoHelper->decode(data, len, pts);
        if (mHwVideoHelper->good()) {
            return;
        } else {
            HEVC_DPRINT("Failed to decode with HW decoder %d; switch to SW",
                        mHwVideoHelper->error());
            mHwVideoHelper.reset(nullptr);
        }
    }

    mTrialPeriod = false;
    createAndInitSoftVideoHelper();
    mVideoHelper = std::move(mSwVideoHelper);

    mVideoHelper->setIgnoreDecodedFrames();
    std::function<void(const uint8_t*, size_t, uint64_t)> func =
            [=](const uint8_t* data, size_t len, uint64_t pts) {
                this->oneShotDecode(data, len, pts);
            };

    mSnapshotHelper->replay(func);

    mVideoHelper->setSaveDecodedFrames();

    mVideoHelper->decode(data, len, pts);
}

void MediaHevcDecoderGeneric::fetchAllFrames() {
    while (true) {
        MediaSnapshotState::FrameInfo frame;
        bool success =
                mHwVideoHelper
                        ? mHwVideoHelper->receiveFrame(&frame)
                        : (mVideoHelper ? mVideoHelper->receiveFrame(&frame)
                                        : false);
        if (!success) {
            break;
        }
        mSnapshotHelper->saveDecodedFrame(std::move(frame));
    }
}

void MediaHevcDecoderGeneric::flush(void* ptr) {
    HEVC_DPRINT("Flushing...");
    if (mHwVideoHelper) {
        mHwVideoHelper->flush();
    } else if (mVideoHelper) {
        mVideoHelper->flush();
    }
    fetchAllFrames();
    HEVC_DPRINT("Flushing done");
}

void MediaHevcDecoderGeneric::sendMetadata(void* ptr) {
    HEVC_DPRINT("hevc %s %d sendMetadata %p\n", __func__, __LINE__, ptr);
    MetadataParam param{};
    mParser.parseMetadataParams(ptr, param);

    bool isValid = false;
    if (param.range == 2 && param.primaries == 4 && param.transfer == 3) {
        isValid = true;
    } else if (param.range == 1 && param.primaries == 4 &&
               param.transfer == 3) {
        isValid = true;
    } else if (param.range == 2 && param.primaries == 1 &&
               param.transfer == 3) {
        isValid = true;
    }

    if (!isValid) {
        HEVC_DPRINT("%s %d invalid values in sendMetadata %p, ignore.\n",
                    __func__, __LINE__, ptr);
        return;
    }

    mMetadataPtr.reset(new MetadataParam(param));

    HEVC_DPRINT("hevc %s %d range %d primaries %d transfer %d\n", __func__,
                __LINE__, (int)(mMetadataPtr->range), (int)(mMetadataPtr->primaries),
                (int)(mMetadataPtr->transfer));
}

void MediaHevcDecoderGeneric::getImage(void* ptr) {
    HEVC_DPRINT("getImage %p", ptr);
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retErr = param.pDecoderErrorCode;
    uint32_t* retWidth = param.pRetWidth;
    uint32_t* retHeight = param.pRetHeight;
    uint64_t* retPts = param.pRetPts;
    uint32_t* retColorPrimaries = param.pRetColorPrimaries;
    uint32_t* retColorRange = param.pRetColorRange;
    uint32_t* retColorTransfer = param.pRetColorTransfer;
    uint32_t* retColorSpace = param.pRetColorSpace;
    uint8_t* dst = param.pDecodedFrame;

    static int numbers = 0;
    HEVC_DPRINT("calling getImage %d colorbuffer %d", numbers++,
                (int)param.hostColorBufferId);

    MediaSnapshotState::FrameInfo* pFrame = mSnapshotHelper->frontFrame();
    if (pFrame == nullptr) {
        HEVC_DPRINT("there is no image");
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }

    // update color aspects
    if (mMetadataPtr && pFrame->color.range != 0) {
        if (3 - pFrame->color.range != mMetadataPtr->range) {
            mMetadataPtr->range = 3 - pFrame->color.range;
        }
    }

    bool needToCopyToGuest = true;
    if (mParser.version() >= 200) {
        if (mUseGpuTexture && pFrame->texture[0] > 0 &&
            pFrame->texture[1] > 0) {
            HEVC_DPRINT(
                    "calling rendering to host side color buffer with id %d "
                    "tex %d tex %d",
                    param.hostColorBufferId, pFrame->texture[0],
                    pFrame->texture[1]);
            mRenderer.renderToHostColorBufferWithTextures(
                    param.hostColorBufferId, pFrame->width, pFrame->height,
                    TextureFrame{pFrame->texture[0], pFrame->texture[1]},
                    mMetadataPtr.get());
        } else {
            HEVC_DPRINT(
                    "calling rendering to host side color buffer with id %d",
                    param.hostColorBufferId);
            mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                              pFrame->width, pFrame->height,
                                              pFrame->data.data(), mMetadataPtr.get());
        }
        needToCopyToGuest = false;
    }

    if (needToCopyToGuest) {
        memcpy(param.pDecodedFrame, pFrame->data.data(),
               pFrame->width * pFrame->height * 3 / 2);
    }

    *retErr = pFrame->width * pFrame->height * 3 / 2;
    *retWidth = pFrame->width;
    *retHeight = pFrame->height;
    *retPts = pFrame->pts;
    *retColorPrimaries = pFrame->color.primaries;
    *retColorRange = pFrame->color.range;
    *retColorSpace = pFrame->color.space;
    *retColorTransfer = pFrame->color.transfer;

    mSnapshotHelper->discardFrontFrame();

    HEVC_DPRINT("Copying completed pts %lld w %d h %d ow %d oh %d",
                (long long)*retPts, (int)*retWidth, (int)*retHeight,
                (int)mOutputWidth, (int)mOutputHeight);
}

void MediaHevcDecoderGeneric::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    stream->putBe32((int)mOutPixFmt);

    const int hasContext = (mVideoHelper || mHwVideoHelper) ? 1 : 0;
    stream->putBe32(hasContext);

    mSnapshotHelper->save(stream);
}

void MediaHevcDecoderGeneric::oneShotDecode(const uint8_t* data,
                                            size_t len,
                                            uint64_t pts) {
    if (!mHwVideoHelper && !mSwVideoHelper && !mVideoHelper) {
        return;
    }

    decodeFrameInternal(data, len, pts);
    while (true) {
        MediaSnapshotState::FrameInfo frame;
        bool success = mHwVideoHelper ? mHwVideoHelper->receiveFrame(&frame)
                                      : mVideoHelper->receiveFrame(&frame);
        if (!success) {
            break;
        }
    }
}

bool MediaHevcDecoderGeneric::load(base::Stream* stream) {
    uint32_t version = stream->getBe32();
    mParser = HevcPingInfoParser{version};

    mWidth = stream->getBe32();
    mHeight = stream->getBe32();
    mOutPixFmt = (PixelFormat)stream->getBe32();

    const int hasContext = stream->getBe32();
    if (hasContext) {
        initHevcContextInternal(mWidth, mHeight, mWidth, mHeight, mOutPixFmt);
    }

    if (mVideoHelper) {
        mVideoHelper->setIgnoreDecodedFrames();
    }
    std::function<void(const uint8_t*, size_t, uint64_t)> func =
            [=](const uint8_t* data, size_t len, uint64_t pts) {
                this->oneShotDecode(data, len, pts);
            };

    mSnapshotHelper->load(stream, func);

    if (mVideoHelper) {
        mVideoHelper->setSaveDecodedFrames();
    }

    HEVC_DPRINT("Done loading snapshots frames\n\n");
    return true;
}

}  // namespace emulation
}  // namespace android
