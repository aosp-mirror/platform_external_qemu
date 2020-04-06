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

#include "android/emulation/MediaCudaUtils.h"
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

#define MEDIA_CUVID_DEBUG 0

#if MEDIA_CUVID_DEBUG
#define CUVID_DPRINT(fmt, ...)                                          \
    fprintf(stderr, "cuvid-utils: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define CUVID_DPRINT(fmt, ...)
#endif

#define NVDEC_API_CALL(cuvidAPI)                                      \
    do {                                                              \
        CUresult errorCode = cuvidAPI;                                \
        if (errorCode != CUDA_SUCCESS) {                              \
            CUVID_DPRINT("%s failed with error code %d\n", #cuvidAPI, \
                         (int)errorCode);                             \
        }                                                             \
    } while (0)

extern "C" {

#define MEDIA_CUDA_COPY_Y_TEXTURE 1
#define MEDIA_CUDA_COPY_UV_TEXTURE 2

static void media_cuda_copy_decoded_frame(void* privData,
                                          int mode,
                                          uint32_t dest_texture_handle) {
    media_cuda_utils_copy_context* copy_context =
            static_cast<media_cuda_utils_copy_context*>(privData);

    const unsigned int GL_TEXTURE_2D = 0x0DE1;
    const unsigned int cudaGraphicsMapFlagsNone = 0x0;
    CUgraphicsResource CudaRes{0};
    CUVID_DPRINT("cuda copy decoded frame testure %d",
                 (int)dest_texture_handle);
    NVDEC_API_CALL(cuGraphicsGLRegisterImage(&CudaRes, dest_texture_handle,
                                             GL_TEXTURE_2D, 0x0));
    CUarray texture_ptr;
    NVDEC_API_CALL(cuGraphicsMapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(
            cuGraphicsSubResourceGetMappedArray(&texture_ptr, CudaRes, 0, 0));
    CUdeviceptr dpSrcFrame = copy_context->src_frame;
    CUDA_MEMCPY2D m = {0};
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = copy_context->src_pitch;
    m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
    m.dstArray = texture_ptr;
    m.dstPitch = copy_context->dest_width * 1;
    m.WidthInBytes = copy_context->dest_width * 1;
    m.Height = copy_context->dest_height;
    CUVID_DPRINT("dstPitch %d, WidthInBytes %d Height %d surface-height %d",
                 (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height,
                 (int)copy_context->src_surface_height);

    if (mode == MEDIA_CUDA_COPY_Y_TEXTURE) {  // copy Y data
        NVDEC_API_CALL(cuMemcpy2D(&m));
    } else if (mode == MEDIA_CUDA_COPY_UV_TEXTURE) {  // copy UV data
        m.srcDevice =
                (CUdeviceptr)((uint8_t*)dpSrcFrame +
                              m.srcPitch * copy_context->src_surface_height);
        m.Height = m.Height / 2;
        NVDEC_API_CALL(cuMemcpy2D(&m));
    }
    NVDEC_API_CALL(cuGraphicsUnmapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(cuGraphicsUnregisterResource(CudaRes));
}

void media_cuda_utils_nv12_updater(void* privData,
                                   uint32_t type,
                                   uint32_t* textures) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    CUVID_DPRINT("copyiong Ytex %d", textures[0]);
    CUVID_DPRINT("copyiong UVtex %d", textures[1]);
    media_cuda_copy_decoded_frame(privData, MEDIA_CUDA_COPY_Y_TEXTURE,
                                  textures[0]);
    media_cuda_copy_decoded_frame(privData, MEDIA_CUDA_COPY_UV_TEXTURE,
                                  textures[1]);
}

}  // end extern C
