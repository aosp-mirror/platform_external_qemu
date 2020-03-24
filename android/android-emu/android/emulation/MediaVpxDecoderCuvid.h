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
#include "android/emulation/MediaCodec.h"
#include "android/emulation/MediaHostRenderer.h"
#include "android/emulation/MediaVpxDecoderPlugin.h"
#include "android/emulation/VpxPingInfoParser.h"

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

class MediaVpxDecoderCuvid : public MediaVpxDecoderPlugin {
public:
    virtual void initVpxContext(void* ptr) override;
    virtual void decodeFrame(void* ptr) override;
    virtual void getImage(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void destroyVpxContext(void* ptr) override;

    explicit MediaVpxDecoderCuvid(VpxPingInfoParser parser,
                                  MediaCodecType type);
    virtual ~MediaVpxDecoderCuvid();

    virtual int type() const override { return PLUGIN_TYPE_CUVID; }

public:
    // cuda related methods
    static bool initCudaDrivers();
    static bool s_isCudaInitialized;

private:
    MediaCodecType mType;  // vp8 or vp9
    VpxPingInfoParser mParser;
    MediaHostRenderer mRenderer;

    uint64_t mOutputPts = 0;

    bool mIsInFlush = false;

private:
    // image props
    bool mUseGpuTexture = false;
    bool mImageReady = false;
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
    unsigned int mOutBufferSize = 0;

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
    void initVpxContextInternal();
    void doFlush();
    void decodeFrameInternal(const uint8_t* frame,
                             size_t szBytes,
                             uint64_t inputPts);

private:
    // cuda call back
    static int CUDAAPI HandleVideoSequenceProc(void* pUserData,
                                               CUVIDEOFORMAT* pVideoFormat) {
        return ((MediaVpxDecoderCuvid*)pUserData)
                ->HandleVideoSequence(pVideoFormat);
    }
    static int CUDAAPI HandlePictureDecodeProc(void* pUserData,
                                               CUVIDPICPARAMS* pPicParams) {
        return ((MediaVpxDecoderCuvid*)pUserData)
                ->HandlePictureDecode(pPicParams);
    }
    static int CUDAAPI
    HandlePictureDisplayProc(void* pUserData, CUVIDPARSERDISPINFO* pDispInfo) {
        return ((MediaVpxDecoderCuvid*)pUserData)
                ->HandlePictureDisplay(pDispInfo);
    }

    int HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat);

    int HandlePictureDecode(CUVIDPICPARAMS* pPicParams);

    int HandlePictureDisplay(CUVIDPARSERDISPINFO* pDispInfo);
};  // MediaVpxDecoderCuvid

}  // namespace emulation
}  // namespace android
