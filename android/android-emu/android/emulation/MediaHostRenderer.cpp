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

#include "android/emulation/MediaHostRenderer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                         \
    fprintf(stderr, "media-host-renderer: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

MediaHostRenderer::MediaHostRenderer() {
    mVirtioGpuOps = android_getVirtioGpuOps();
    if (mVirtioGpuOps == nullptr) {
        H264_DPRINT("Error, cannot get mVirtioGpuOps");
    }
}

const uint32_t kGlUnsignedByte = 0x1401;

constexpr uint32_t kGL_RGBA8 = 0x8058;

void MediaHostRenderer::renderToHostColorBuffer(int hostColorBufferId,
                                                unsigned int outputWidth,
                                                unsigned int outputHeight,
                                                uint8_t* decodedFrame) {
    H264_DPRINT("Calling %s at %d buffer id %d", __func__, __LINE__,
                hostColorBufferId);
    if (hostColorBufferId < 0) {
        H264_DPRINT("ERROR: negative buffer id %d", hostColorBufferId);
        return;
    }
    if (mVirtioGpuOps) {
        mVirtioGpuOps->update_color_buffer(hostColorBufferId, 0, 0, outputWidth,
                                           outputHeight, kGL_RGBA8,
                                           kGlUnsignedByte, decodedFrame);
    } else {
        H264_DPRINT("ERROR: there is no virtio Gpu Ops is not setup");
    }
}

}  // namespace emulation
}  // namespace android
