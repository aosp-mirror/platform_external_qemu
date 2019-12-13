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

#include <cuda.h>
#include <nvcuvid.h>
#include <cuviddec.h>
#include "android/utils/dll.h"

namespace android {
namespace emulation {

struct CudaVideoFunctions {
    // cuda
    CUresult (*cuInit)(unsigned int  Flags);
    CUresult (*cuDeviceGetCount) ( int* count );
    CUresult (*cuDeviceGetName)(char *name, int len, CUdevice dev);
    CUresult (*cuDeviceGet)(CUdevice *device, int ordinal);
    CUresult (*cuCtxPushCurrent) ( CUcontext ctx );
    CUresult (*cuCtxPopCurrent) ( CUcontext* pctx );
    CUresult (*cuCtxCreate)(CUcontext *pctx, unsigned int flags, CUdevice dev);
    CUresult (*cuCtxDestroy)(CUcontext ctx);
    CUresult (*cuMemcpy2DAsync) ( const CUDA_MEMCPY2D* pCopy, CUstream hStream );
    CUresult (*cuStreamSynchronize) ( CUstream hStream );
    CUresult (*cuGetErrorName)(CUresult error, const char **pStr);
    CUresult (*cuMemGetInfo)(unsigned int *free, unsigned int *total);

    // decoder
    CUresult (*cuvidGetDecoderCaps)(CUVIDDECODECAPS *pdc);
    CUresult (*cuvidCreateDecoder)(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
    CUresult (*cuvidDecodePicture)(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);
    CUresult (*cuvidGetDecodeStatus)(CUvideodecoder hDecoder, int nPicIdx, CUVIDGETDECODESTATUS* pDecodeStatus);
    CUresult (*cuvidDestroyDecoder)(CUvideodecoder hDecoder);

    // map and unmap
    CUresult (*cuvidMapVideoFrame)(CUvideodecoder hDecoder, int nPicIdx,
                                           unsigned long long *pDevPtr, unsigned int *pPitch,
                                           CUVIDPROCPARAMS *pVPP);
    CUresult (*cuvidUnmapVideoFrame)(CUvideodecoder hDecoder, unsigned int DevPtr);

    // lock
    CUresult (*cuvidCtxLockCreate)(CUvideoctxlock *pLock, CUcontext ctx);
    CUresult (*cuvidCtxLockDestroy)(CUvideoctxlock lck);

    // parser
    CUresult (*cuvidCreateVideoParser)(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
    CUresult (*cuvidParseVideoData)(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);
    CUresult (*cuvidDestroyVideoParser)(CUvideoparser obj);

};

class CudaVideoLibraryLoader {
public:
    CudaVideoLibraryLoader() = default;
    ~CudaVideoLibraryLoader() = default;

// important libraries needed for video decoding
// 1. libcuda
// 2. libnvcuvid.so
    CudaVideoFunctions* getFunctions() {
        if (mSuccess) return &mFunctions;
        else return nullptr;
    }

    bool openLibrary();
    void print(CudaVideoFunctions* funcs);

private:
    void installFuncPtr(ADynamicLibrary* dlib, void** pfunc, const char* name);

    CudaVideoFunctions mFunctions = {0};

    bool mSuccess = false;
};

}  // namespace emulation
}  // namespace android
