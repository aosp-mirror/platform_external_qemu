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

#include "android/emulation/MediaVideoToolBoxUtils.h"

#define MEDIA_VTB_DEBUG 1

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

typedef void (*nsConvertVideoFrameToNV12TexturesFun)(void* context, void* iosurface, int* Ytex, int* UVtex);
typedef void (*nsCopyTextureFun)(void* context, int from, int to, int w, int h);

struct CallerData {
    void* ctx;
    void* f1;
    void* f2;
};

void media_vtb_utils_nv12_updater(void* privData,
                                   uint32_t type,
                                   uint32_t* textures,
                                   void* callerData) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    // get the textures
    media_vtb_utils_copy_context* myctx = (media_vtb_utils_copy_context*)privData; 
    int ww = myctx->dest_width;
    int hh = myctx->dest_height;
    void* iosurface = myctx->iosurface;
    int yy=0;
    int uv=0;

    CallerData* cdata = (struct CallerData*)callerData;

    void* nscontext = cdata->ctx;
    auto nsConvertVideoFrameToNV12Textures = (nsConvertVideoFrameToNV12TexturesFun)cdata->f1;
    auto nsCopyTexture = (nsCopyTextureFun)cdata->f2;
    VTB_DPRINT("create Ytex and UV tex func %p context %p surface %p w %d h %d",
            nsConvertVideoFrameToNV12Textures, nscontext, iosurface, ww, hh);
    nsConvertVideoFrameToNV12Textures(nscontext, iosurface, &yy, &uv);


    VTB_DPRINT("copyiong Ytex %d", textures[0]);
    nsCopyTexture(nscontext, yy, textures[0], ww, hh);
   // VTB_DPRINT("copyiong UVtex %d", textures[1]);
   // nsCopyTexture(nscontext, uv, textures[0], ww/2, hh/2);
    VTB_DPRINT("done ");
}


} // end of extern C
