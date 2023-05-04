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

#include "host-common/MediaVideoToolBoxUtils.h"
#include "render-utils/MediaNative.h"

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

void media_vtb_utils_nv12_updater(void* privData,
                                  uint32_t type,
                                  uint32_t* textures,
                                  void* callerData) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    // get the textures
    media_vtb_utils_copy_context* myctx =
            (media_vtb_utils_copy_context*)privData;
    int ww = myctx->dest_width;
    int hh = myctx->dest_height;
    void* iosurface = myctx->iosurface;
    int yy = 0;
    int uv = 0;

    MediaNativeCallerData* cdata = (struct MediaNativeCallerData*)callerData;

    void* nscontext = cdata->ctx;
    auto converter = cdata->converter;
    converter(nscontext, iosurface, textures[0], textures[1]);

    VTB_DPRINT("done ");
}

}  // end of extern C
