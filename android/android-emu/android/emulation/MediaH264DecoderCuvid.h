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

#pragma once

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/H264PingInfoParser.h"
#include "android/emulation/MediaCodec.h"
#include "android/emulation/MediaH264DecoderPlugin.h"
#include "android/emulation/MediaHostRenderer.h"

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaH264DecoderCuvid : public MediaH264DecoderPlugin {
public:
    virtual void reset(void* ptr) override;
    virtual void initH264Context(void* ptr) override;
    virtual MediaH264DecoderPlugin* clone() override;
    virtual void destroyH264Context() override;
    virtual void decodeFrame(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;

    explicit MediaH264DecoderCuvid(uint64_t id, H264PingInfoParser parser);
    virtual ~MediaH264DecoderCuvid();

    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

    virtual int type() const override { return PLUGIN_TYPE_CUVID; }

private:
    void initH264ContextInternal(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt);
    uint32_t mId = 0;
    H264PingInfoParser mParser;
    MediaHostRenderer mRenderer;

    // void decodeFrameInternal(void* ptr, const uint8_t* frame, size_t szBytes,
    // uint64_t pts, size_t consumedSzBytes);
    // We should move these shared memory calls elsewhere, as vpx decoder is
    // also using the same/similar functions
    static void* getReturnAddress(void* ptr);
    static uint8_t* getDst(void* ptr);

    uint64_t mOutputPts = 0;
    std::vector<uint8_t> mSPS;  // sps NALU
    std::vector<uint8_t> mPPS;  // pps NALU

    bool mIsInFlush = false;

public:
    // cuda related methods
    static bool initCudaDrivers();
    static bool s_isCudaInitialized;

private:
    // cuda call back
    static int CUDAAPI HandleVideoSequenceProc(void* pUserData,
                                               CUVIDEOFORMAT* pVideoFormat) {
        return ((MediaH264DecoderCuvid*)pUserData)
                ->HandleVideoSequence(pVideoFormat);
    }
    static int CUDAAPI HandlePictureDecodeProc(void* pUserData,
                                               CUVIDPICPARAMS* pPicParams) {
        return ((MediaH264DecoderCuvid*)pUserData)
                ->HandlePictureDecode(pPicParams);
    }
    static int CUDAAPI
    HandlePictureDisplayProc(void* pUserData, CUVIDPARSERDISPINFO* pDispInfo) {
        return ((MediaH264DecoderCuvid*)pUserData)
                ->HandlePictureDisplay(pDispInfo);
    }

    int HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat);

    int HandlePictureDecode(CUVIDPICPARAMS* pPicParams);

    int HandlePictureDisplay(CUVIDPARSERDISPINFO* pDispInfo);

    void doFlush();

private:
    // image props
    bool mUseGpuTexture = false;
    bool mImageReady = false;
    static constexpr int kBPP = 2;  // YUV420 is 2 bytes per pixel
    unsigned int mHeight = 0;
    unsigned int mWidth = 0;
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = 0;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    PixelFormat mOutPixFmt;
    unsigned int mOutBufferSize = 0;

    unsigned int mColorPrimaries = 2;
    unsigned int mColorRange = 0;
    unsigned int mColorTransfer = 2;
    unsigned int mColorSpace = 2;

    // right now, decoding command only passes the input address;
    // and output address is only available in getImage().
    // TODO: this should be set to the output address to avoid
    // extra copying
    mutable std::list<int> mSavedW;
    mutable std::list<int> mSavedH;
    mutable std::list<uint64_t> mSavedPts;
    // mSavedFrames are byte frames saved on CPU
    mutable std::list<std::vector<uint8_t>> mSavedFrames;
    // mSavedTexFrames are texture frames saved on GPU
    mutable std::list<MediaHostRenderer::TextureFrame> mSavedTexFrames;
    std::mutex mFrameLock;

    // cuda stuff
    CUcontext mCudaContext = nullptr;
    CUvideoctxlock mCtxLock;
    CUvideoparser mCudaParser = nullptr;
    CUvideodecoder mCudaDecoder = nullptr;

private:
    bool mIsLoadingFromSnapshot = false;
    mutable SnapshotState mSnapshotState;
    void oneShotDecode(std::vector<uint8_t>& data, uint64_t pts);

    void decodeFrameInternal(uint64_t* pRetSzBytes,
                             int32_t* pRetErr,
                             const uint8_t* frame,
                             size_t szBytes,
                             uint64_t inputPts);
};  // MediaH264DecoderCuvid
}  // namespace emulation
}  // namespace android
