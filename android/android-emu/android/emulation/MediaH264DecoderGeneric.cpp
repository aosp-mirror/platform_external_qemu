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

#include "android/emulation/MediaH264DecoderGeneric.h"
#include "android/base/system/System.h"
#include "android/emulation/H264PingInfoParser.h"
#include "android/emulation/MediaFfmpegVideoHelper.h"
#include "android/main-emugl.h"

#ifndef __APPLE__
// for Linux and Window, Cuvid is available
#include "android/emulation/MediaCudaDriverHelper.h"
#include "android/emulation/MediaCudaVideoHelper.h"
#endif

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                                \
    fprintf(stderr, "h264-dec-generic: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using InitContextParam = H264PingInfoParser::InitContextParam;
using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
using ResetParam = H264PingInfoParser::ResetParam;
using GetImageParam = H264PingInfoParser::GetImageParam;
using TextureFrame = MediaHostRenderer::TextureFrame;

namespace {

bool canUseCudaDecoder() {
#ifndef __APPLE__
    if (MediaCudaDriverHelper::initCudaDrivers()) {
        H264_DPRINT("Using Cuvid decoder on Linux/Windows");
        return true;
    } else {
        H264_DPRINT(
                "ERROR: cannot use cuvid decoder: failed to init cuda driver");
        return false;
    }
#else
    return false;
#endif
}

bool canDecodeToGpuTexture() {
#ifndef __APPLE__
    if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
        return true;
    } else {
        return false;
    }
#else
    return false;
#endif
}
};  // end namespace

MediaH264DecoderGeneric::MediaH264DecoderGeneric(uint64_t id,
                                                 H264PingInfoParser parser)
    : mId(id), mParser(parser) {
    H264_DPRINT("allocated MediaH264DecoderGeneric %p with version %d", this,
                (int)mParser.version());
    mUseGpuTexture = canDecodeToGpuTexture();
}

MediaH264DecoderGeneric::~MediaH264DecoderGeneric() {
    H264_DPRINT("destroyed MediaH264DecoderGeneric %p", this);
    destroyH264Context();
}

void MediaH264DecoderGeneric::reset(void* ptr) {
    destroyH264Context();
    ResetParam param{};
    mParser.parseResetParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderGeneric::initH264Context(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderGeneric::initH264ContextInternal(unsigned int width,
                                                      unsigned int height,
                                                      unsigned int outWidth,
                                                      unsigned int outHeight,
                                                      PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)", __func__, width,
                height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;

#ifndef __APPLE__
    if (canUseCudaDecoder() && mParser.version() >= 200) {
        MediaCudaVideoHelper::OutputTreatmentMode oMode =
                MediaCudaVideoHelper::OutputTreatmentMode::SAVE_RESULT;

        MediaCudaVideoHelper::FrameStorageMode fMode =
                mUseGpuTexture ? MediaCudaVideoHelper::FrameStorageMode::
                                         USE_GPU_TEXTURE
                               : MediaCudaVideoHelper::FrameStorageMode::
                                         USE_BYTE_BUFFER;

        auto cudavid =
                new MediaCudaVideoHelper(oMode, fMode, cudaVideoCodec_H264);

        if (mUseGpuTexture) {
            H264_DPRINT("use gpu texture");
            cudavid->resetTexturePool(mRenderer.getTexturePool());
        }
        mHwVideoHelper.reset(cudavid);
        if (!mHwVideoHelper->init()) {
            mHwVideoHelper.reset(nullptr);
            H264_DPRINT("failed to init cuda decoder");
        } else {
            H264_DPRINT("succeeded to init cuda decoder");
        }
    }
// TODO: add video toolbox for apple
#endif

    mSnapshotHelper.reset(
            new MediaSnapshotHelper(MediaSnapshotHelper::CodecType::H264));

    H264_DPRINT("Successfully created h264 decoder context %p", this);
}

void MediaH264DecoderGeneric::createAndInitSoftVideoHelper() {
    mSwVideoHelper.reset(
            new MediaFfmpegVideoHelper(264, mParser.version() < 200 ? 1 : 4));
    mUseGpuTexture = false;
    mSwVideoHelper->init();
}

MediaH264DecoderPlugin* MediaH264DecoderGeneric::clone() {
    H264_DPRINT("clone MediaH264DecoderGeneric %p with version %d", this,
                (int)mParser.version());
    return new MediaH264DecoderGeneric(mId, mParser);
}

void MediaH264DecoderGeneric::destroyH264Context() {
    H264_DPRINT("Destroy context %p", this);
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

void MediaH264DecoderGeneric::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);
    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t inputPts = param.pts;

    H264_DPRINT("%s(frame=%p, sz=%zu pts %lld)", __func__, frame, szBytes,
                (long long)inputPts);
    size_t* retSzBytes = param.pConsumedBytes;
    int32_t* retErr = param.pDecoderErrorCode;

    decodeFrameInternal(frame, szBytes, inputPts);

    mSnapshotHelper->savePacket(frame, szBytes, inputPts);
    fetchAllFrames();

    *retSzBytes = szBytes;
    *retErr = (int32_t)Err::NoErr;
}

void MediaH264DecoderGeneric::decodeFrameInternal(const uint8_t* data,
                                                  size_t len,
                                                  uint64_t pts) {
    if (mTrialPeriod) {
        try_decode(data, len, pts);
    } else {
        mVideoHelper->decode(data, len, pts);
    }
}

void MediaH264DecoderGeneric::try_decode(const uint8_t* data,
                                         size_t len,
                                         uint64_t pts) {
    // for h264, it needs sps/pps to decide whether decoding is
    // possible;
    if (mHwVideoHelper != nullptr) {
        mHwVideoHelper->decode(data, len, pts);
        if (mHwVideoHelper->good()) {
            return;
        } else {
            H264_DPRINT("Failed to decode with HW decoder %d; switch to SW",
                        mHwVideoHelper->error());
            mHwVideoHelper.reset(nullptr);
        }
    }

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
    mTrialPeriod = false;
}

void MediaH264DecoderGeneric::fetchAllFrames() {
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

void MediaH264DecoderGeneric::flush(void* ptr) {
    H264_DPRINT("Flushing...");
    if (mHwVideoHelper) {
        mHwVideoHelper->flush();
    } else if (mVideoHelper) {
        mVideoHelper->flush();
    }
    fetchAllFrames();
    H264_DPRINT("Flushing done");
}

void MediaH264DecoderGeneric::getImage(void* ptr) {
    H264_DPRINT("getImage %p", ptr);
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
    H264_DPRINT("calling getImage %d colorbuffer %d", numbers++,
                (int)param.hostColorBufferId);

    MediaSnapshotState::FrameInfo* pFrame = mSnapshotHelper->frontFrame();
    if (pFrame == nullptr) {
        H264_DPRINT("there is no image");
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }

    bool needToCopyToGuest = true;
    if (mParser.version() == 200) {
        if (mUseGpuTexture && pFrame->texture[0] > 0 && pFrame->texture[1] > 0) {
            mRenderer.renderToHostColorBufferWithTextures(
                    param.hostColorBufferId, pFrame->width, pFrame->height,
                    TextureFrame{pFrame->texture[0], pFrame->texture[1]});
        } else {
            H264_DPRINT(
                    "calling rendering to host side color buffer with id %d",
                    param.hostColorBufferId);
            mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                              pFrame->width, pFrame->height,
                                              pFrame->data.data());
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

    H264_DPRINT("Copying completed pts %lld w %d h %d ow %d oh %d",
                (long long)*retPts, (int)*retWidth, (int)*retHeight,
                (int)mOutputWidth, (int)mOutputHeight);
}

void MediaH264DecoderGeneric::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    stream->putBe32((int)mOutPixFmt);

    const int hasContext = (mVideoHelper || mHwVideoHelper) ? 1 : 0;
    stream->putBe32(hasContext);

    mSnapshotHelper->save(stream);
}

void MediaH264DecoderGeneric::oneShotDecode(const uint8_t* data,
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

bool MediaH264DecoderGeneric::load(base::Stream* stream) {
    uint32_t version = stream->getBe32();
    mParser = H264PingInfoParser{version};

    mWidth = stream->getBe32();
    mHeight = stream->getBe32();
    mOutPixFmt = (PixelFormat)stream->getBe32();

    const int hasContext = stream->getBe32();
    if (hasContext) {
        initH264ContextInternal(mWidth, mHeight, mWidth, mHeight, mOutPixFmt);
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

    H264_DPRINT("Done loading snapshots frames\n\n");
    return true;
}

}  // namespace emulation
}  // namespace android
