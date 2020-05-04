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

#include "android/emulation/MediaCudaDriverHelper.h"
#include <cstdint>
#include <string>
#include <vector>
#include "android/main-emugl.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#endif

#include <stdio.h>
#include <string.h>

extern "C" {
#define INIT_CUDA_GL 1
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_cudaGL.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#define MEDIA_CUDA_DEBUG 0

#if MEDIA_CUDA_DEBUG
#define CUDA_DPRINT(fmt, ...)                                               \
    fprintf(stderr, "media-cuda--driver-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define CUDA_DPRINT(fmt, ...)
#endif

#define NVDEC_API_CALL(cuvidAPI)                                     \
    do {                                                             \
        CUresult errorCode = cuvidAPI;                               \
        if (errorCode != CUDA_SUCCESS) {                             \
            CUDA_DPRINT("%s failed with error code %d\n", #cuvidAPI, \
                        (int)errorCode);                             \
        }                                                            \
    } while (0)

namespace android {
namespace emulation {

bool MediaCudaDriverHelper::s_isCudaInitialized = false;

bool MediaCudaDriverHelper::initCudaDrivers() {
    if (s_isCudaInitialized) {
        return true;
    }
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void* CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    int initResult = CUDA_SUCCESS;
    // try 3 times: especially important after reboot
    // the cuda ko might not be loaded in the firs try
    for (int i = 0; i < 3; ++ i) {
        initResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
        if (CUDA_SUCCESS == initResult) {
            break;
        }
    }

    if (initResult != CUDA_SUCCESS) {
        fprintf(stderr,
                "Failed to call cuInit, cannot use nvidia cuvid decoder for "
                "h264 stream\n");
        return false;
    }

    if (CUDA_SUCCESS != cuvidInit(0)) {
        fprintf(stderr,
                "Failed to call cuvidInit, cannot use nvidia cuvid decoder for "
                "h264 stream\n");
        return false;
    }

    int numGpuCards = 0;
    CUresult myres = cuDeviceGetCount(&numGpuCards);
    if (myres != CUDA_SUCCESS) {
        CUDA_DPRINT(
                "Failed to get number of GPU cards installed on host; error "
                "code %d",
                (int)myres);
        return false;
    }

    if (numGpuCards <= 0) {
        CUDA_DPRINT("There are no nvidia GPU cards on this host.");
        return false;
    }

    // lukily, we get cuda initialized.
    s_isCudaInitialized = true;

    return true;
}

}  // namespace emulation
}  // namespace android
