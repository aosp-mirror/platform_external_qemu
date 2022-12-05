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

#include "host-common/MediaHostRenderer.h"

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

using TextureFrame = MediaTexturePool::TextureFrame;

MediaHostRenderer::MediaHostRenderer() {
    mVirtioGpuOps = android_getVirtioGpuOps();
    if (mVirtioGpuOps == nullptr) {
        H264_DPRINT("Error, cannot get mVirtioGpuOps");
    }
}

MediaHostRenderer::~MediaHostRenderer() {
    cleanUpTextures();
}

constexpr uint32_t kFrameworkFormatYuv420888 = 0x2; // FRAMEWORK_FORMAT_YUV_420_888;

const uint32_t kGlUnsignedByte = 0x1401;

constexpr uint32_t kGL_RGBA8 = 0x8058;
constexpr uint32_t kGL_RGBA = 0x1908;
constexpr uint32_t kFRAME_POOL_SIZE = 8;
constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;

TextureFrame MediaHostRenderer::getTextureFrame(int w, int h) {
    return mTexturePool.getTextureFrame(w, h);
}

void MediaHostRenderer::saveDecodedFrameToTexture(TextureFrame frame,
                                                  void* privData,
                                                  void* func) {
    mTexturePool.saveDecodedFrameToTexture(frame, privData, func);
}

void MediaHostRenderer::cleanUpTextures() {
    mTexturePool.cleanUpTextures();
}

// TODO: add metadata ptr default nullptr
//  Host byte buffer upload is always I420, so kFrameworkFormatYuv420888 is
//  used.
void MediaHostRenderer::renderToHostColorBuffer(int hostColorBufferId,
                                                unsigned int outputWidth,
                                                unsigned int outputHeight,
                                                uint8_t* decodedFrame,
                                                void* metadata) {
    H264_DPRINT("Calling %s at %d buffer id %d", __func__, __LINE__,
                hostColorBufferId);
    if (hostColorBufferId < 0) {
        H264_DPRINT("ERROR: negative buffer id %d", hostColorBufferId);
        return;
    }
    if (mVirtioGpuOps) {
        mVirtioGpuOps->update_color_buffer_from_framework_format(
                hostColorBufferId, 0, 0, outputWidth, outputHeight,
                kFrameworkFormatYuv420888, kGL_RGBA, kGlUnsignedByte,
                decodedFrame, metadata);
    } else {
        H264_DPRINT("ERROR: there is no virtio Gpu Ops is not setup");
    }
}

//  NV12 implicitly. YOLO, doesn't consider target color buffer's actual format
//  if differs, but we don't expect it to differ (crosses fingers, knocks on
//  wood, famous last words)
void MediaHostRenderer::renderToHostColorBufferWithTextures(
        int hostColorBufferId,
        unsigned int outputWidth,
        unsigned int outputHeight,
        TextureFrame frame,
        void* metadata) {
    H264_DPRINT("Calling %s at %d buffer id %d", __func__, __LINE__,
                hostColorBufferId);
    if (hostColorBufferId < 0) {
        H264_DPRINT("ERROR: negative buffer id %d", hostColorBufferId);
        // try to recycle valid textures: this could happen
        mTexturePool.putTextureFrame(frame);
        return;
    }
    if (frame.Ytex <= 0 || frame.UVtex <= 0) {
        H264_DPRINT("ERROR: invalid tex ids: Ytex %d UVtex %d", (int)frame.Ytex,
                    (int)frame.UVtex);
        return;
    }
    if (mVirtioGpuOps) {
        uint32_t textures[2] = {frame.Ytex, frame.UVtex};
        mVirtioGpuOps->swap_textures_and_update_color_buffer(
                hostColorBufferId, 0, 0, outputWidth, outputHeight, kGL_RGBA,
                kGlUnsignedByte, kFRAMEWORK_FORMAT_NV12, textures, metadata);
        if (textures[0] > 0 && textures[1] > 0) {
            frame.Ytex = textures[0];
            frame.UVtex = textures[1];
            H264_DPRINT("return textures to pool %d %d", frame.Ytex,
                        frame.UVtex);
            mTexturePool.putTextureFrame(frame);
        }
    } else {
        H264_DPRINT("ERROR: there is no virtio Gpu Ops is not setup");
    }
}

}  // namespace emulation
}  // namespace android
