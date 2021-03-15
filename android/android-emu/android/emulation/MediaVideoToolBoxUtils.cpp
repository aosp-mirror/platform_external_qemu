// Copyright (C) 2021 The Android Open Source Project
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

#define MEDIA_VTB_DEBUG 0

#if MEDIA_VTB_DEBUG
#define VTB_DPRINT(fmt, ...)                                          \
    fprintf(stderr, "vtb-utils: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define VTB_DPRINT(fmt, ...)
#endif

extern "C" {
#define MEDIA_VTB_COPY_Y_TEXTURE 1
#define MEDIA_VTB_COPY_UV_TEXTURE 2

static void media_vtb_copy_decoded_frame(void* privData, int mode, uint32_t* texture) {
}

void media_vtb_utils_nv12_updater(void* privData,
                                   uint32_t type,
                                   uint32_t* textures) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    VTB_DPRINT("copyiong Ytex %d", textures[0]);
    VTB_DPRINT("copyiong UVtex %d", textures[1]);
    media_vtb_copy_decoded_frame(privData, MEDIA_VTB_COPY_Y_TEXTURE,
                                  textures[0]);
    media_vtb_copy_decoded_frame(privData, MEDIA_VTB_COPY_UV_TEXTURE,
                                  textures[1]);
}


} // end of extern C
