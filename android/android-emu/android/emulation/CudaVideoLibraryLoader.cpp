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

#include "android/emulation/CudaVideoLibraryLoader.h"

#include <stdio.h>

namespace android {
namespace emulation {

bool CudaVideoLibraryLoader::openLibrary() {
    mSuccess = true; 
    char* error = NULL;
    const char *CUDALIB = "libcuda";
    // 1. load libcuda first
    ADynamicLibrary* mCudaLib = adynamicLibrary_open(CUDALIB, &error);
    if (!mCudaLib) {
        fprintf(stderr, "Failed to open libcuda, bad\n");
        mSuccess = false;
        return false;
    }
    // 2. get all the cuda funcs

    const char *CODECLIB = "libnvcuvid";
    ADynamicLibrary* mCodecLib = adynamicLibrary_open(CODECLIB, &error);
    if (!mCodecLib) {
        fprintf(stderr, "Failed to open libnvcuvid, bad\n");
        mSuccess = false;
        return false;
    }
    // 3. get all the codec funcs

    // local cuda functions
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuInit), "cuInit");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuDeviceGetCount), "cuDeviceGetCount");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuCtxCreate), "cuCtxCreate");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuCtxPushCurrent), "cuCtxPushCurrent");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuCtxPopCurrent), "cuCtxPopCurrent");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuMemcpy2DAsync), "cuMemcpy2DAsync");
    installFuncPtr(mCudaLib, (void**)&(mFunctions.cuStreamSynchronize), "cuStreamSynchronize");

    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidGetDecoderCaps), "cuvidGetDecoderCaps");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidCreateDecoder), "cuvidCreateDecoder");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidDecodePicture), "cuvidDecodePicture");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidGetDecodeStatus), "cuvidGetDecodeStatus");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidDestroyDecoder), "cuvidDestroyDecoder");

    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidMapVideoFrame), "cuvidMapVideoFrame");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidUnmapVideoFrame), "cuvidUnmapVideoFrame");

    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidCtxLockCreate), "cuvidCtxLockCreate");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidCtxLockDestroy), "cuvidCtxLockDestroy");

    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidCreateVideoParser), "cuvidCreateVideoParser");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidParseVideoData), "cuvidParseVideoData");
    installFuncPtr(mCodecLib, (void**)&(mFunctions.cuvidDestroyVideoParser), "cuvidDestroyVideoParser");

    return mSuccess;
}

void CudaVideoLibraryLoader::installFuncPtr(ADynamicLibrary* dlib, void** pfunc, const char* name) {
    char* error = NULL;
    void* pvoid = adynamicLibrary_findSymbol(dlib, name, &error);
    if (pvoid) {
        long long *myFunc = (long long *)(*pfunc);
        myFunc = (long long *)pvoid;
    } else {
        //this is bad
        fprintf(stderr, "FAILED!!! to get function pointer for %s !!!\n", name);
        mSuccess = false;
    }
}

}  // namespace emulation
}  // namespace android
