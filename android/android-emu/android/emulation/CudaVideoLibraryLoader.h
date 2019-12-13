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

namespace android {
namespace emulation {

struct CudaVideoFunctions {
    CUresult (cuInit*)(unsigned int  Flags);
    CUresult (cuDeviceGetCount*) ( int* count );
    CUresult (cuCtxPushCurrent*) ( CUcontext ctx );
    CUresult (cuCtxPopCurrent*) ( CUcontext* pctx );
    CUresult (cuCtxCreate*)(CUcontext *pctx, unsigned int flags, CUdevice dev);
    CUresult (cuMemcpy2DAsync*) ( const CUDA_MEMCPY2D* pCopy, CUstream hStream );
    CUresult (cuStreamSynchronize*) ( CUstream hStream );

    CUresult (cuvidGetDecoderCaps*)(CUVIDDECODECAPS *pdc);
    CUresult (cuvidCreateDecoder*)(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
    CUresult (cuvidDestroyDecoder*)(CUvideodecoder hDecoder);
    CUresult (cuvidDecodePicture*)(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);
    CUresult (cuvidGetDecodeStatus*)(CUvideodecoder hDecoder, int nPicIdx, CUVIDGETDECODESTATUS* pDecodeStatus);
    CUresult (cuvidMapVideoFrame*)(CUvideodecoder hDecoder, int nPicIdx,
                                           unsigned int *pDevPtr, unsigned int *pPitch,
                                           CUVIDPROCPARAMS *pVPP);
    CUresult (cuvidUnmapVideoFrame*)(CUvideodecoder hDecoder, unsigned int DevPtr);
    CUresult (cuvidCtxLockCreate*)(CUvideoctxlock *pLock, CUcontext ctx);
    CUresult (cuvidCtxLockDestroy*)(CUvideoctxlock lck);
    CUresult (cuvidCreateVideoParser*)(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
    CUresult (cuvidParseVideoData*)(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);

};

class CudaVideoLibraryLoader {
    CudaVideoLibraryLoader() = default;
    ~CudaVideoLibraryLoader();

// important libraries needed for video decoding
// 1. libcuda
// 2. libnvcuvid.so
    CudaVideoFunctions* getFunctions();

private:
    bool openLibrary();

    CudaVideoFunctions mFunctions = {nullptr};

};

}  // namespace emulation
}  // namespace android
