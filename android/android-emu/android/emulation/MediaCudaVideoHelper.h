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

#include "android/emulation/MediaSnapshotState.h"
#include "android/emulation/MediaTexturePool.h"
#include "android/emulation/MediaVideoHelper.h"
#include "android/emulation/YuvConverter.h"

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

class MediaCudaVideoHelper : public MediaVideoHelper {
public:
    enum class FrameStorageMode {
        USE_BYTE_BUFFER = 1,
        USE_GPU_TEXTURE = 2,
    };

    enum class OutputTreatmentMode {
        IGNORE_RESULT = 1,
        SAVE_RESULT = 2,
    };

public:
    MediaCudaVideoHelper(OutputTreatmentMode outMode,
                         FrameStorageMode fMode,
                         cudaVideoCodec cudaVideoCodecType);
    ~MediaCudaVideoHelper() override;

public:
    // return true if success; false otherwise
    bool init() override;
    void decode(const uint8_t* frame,
                size_t szBytes,
                uint64_t inputPts) override;
    void flush() override;
    void deInit() override;

    void resetTexturePool(MediaTexturePool* pool = nullptr) {
        mTexturePool = pool;
    }

    virtual int error() const override { return mErrorCode; }
    virtual bool good() const override { return mIsGood; }
    virtual bool fatal() const override { return false == s_isCudaDecoderGood; }

private:
    static bool s_isCudaDecoderGood;

    uint64_t mNumInputFrame{0};
    uint64_t mNumOutputFrame{0};
    int mErrorCode = 0;
    bool mIsGood = true;
    bool mUseGpuTexture = false;
    // note: MediaCudaVideoHelper does not own the texture Pool
    // don't delete it.
    MediaTexturePool* mTexturePool = nullptr;
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

    unsigned int mColorPrimaries = 2;
    unsigned int mColorRange = 0;
    unsigned int mColorTransfer = 2;
    unsigned int mColorSpace = 2;

    std::mutex mFrameLock;

    // cuda stuff
    CUcontext mCudaContext = nullptr;
    CUvideoctxlock mCtxLock;
    CUvideoparser mCudaParser = nullptr;
    CUvideodecoder mCudaDecoder = nullptr;

    cudaVideoCodec mCudaVideoCodecType;

private:
    // cuda call back
    static int CUDAAPI HandleVideoSequenceProc(void* pUserData,
                                               CUVIDEOFORMAT* pVideoFormat) {
        return ((MediaCudaVideoHelper*)pUserData)
                ->HandleVideoSequence(pVideoFormat);
    }
    static int CUDAAPI HandlePictureDecodeProc(void* pUserData,
                                               CUVIDPICPARAMS* pPicParams) {
        return ((MediaCudaVideoHelper*)pUserData)
                ->HandlePictureDecode(pPicParams);
    }
    static int CUDAAPI
    HandlePictureDisplayProc(void* pUserData, CUVIDPARSERDISPINFO* pDispInfo) {
        return ((MediaCudaVideoHelper*)pUserData)
                ->HandlePictureDisplay(pDispInfo);
    }

    int HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat);

    int HandlePictureDecode(CUVIDPICPARAMS* pPicParams);

    int HandlePictureDisplay(CUVIDPARSERDISPINFO* pDispInfo);

};  // MediaCudaVideoHelper

}  // namespace emulation
}  // namespace android
