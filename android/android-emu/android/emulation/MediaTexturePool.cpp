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

#include "android/emulation/MediaTexturePool.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                                 \
    fprintf(stderr, "media-texture-pool: %s:%d myid: %d " fmt "\n", __func__, \
            __LINE__, m_id, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

static int s_texturePoolId = 0;
MediaTexturePool::MediaTexturePool() {
    mVirtioGpuOps = android_getVirtioGpuOps();
    if (mVirtioGpuOps == nullptr) {
        H264_DPRINT("Error, cannot get mVirtioGpuOps");
    }
    m_id = s_texturePoolId++;
    H264_DPRINT("created texturepool");
}

MediaTexturePool::~MediaTexturePool() {
    H264_DPRINT("destroyed texturepool");
    cleanUpTextures();
}

const uint32_t kGlUnsignedByte = 0x1401;

constexpr uint32_t kGL_RGBA8 = 0x8058;
constexpr uint32_t kGL_RGBA = 0x1908;
constexpr uint32_t kFRAME_POOL_SIZE = 8;
constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;

MediaTexturePool::TextureFrame MediaTexturePool::getTextureFrame(int w, int h) {
    H264_DPRINT("calling %s %d for tex of w %d h %d\n", __func__, __LINE__, w,
                h);
    PoolHandle ph = m_WH_to_PoolHandle[TexSizes{w, h}];
    if (ph == nullptr) {
        ph = new Pool;
        m_WH_to_PoolHandle[TexSizes{w, h}] = ph;
    }
    if (ph->empty()) {
        std::vector<uint32_t> textures(2 * kFRAME_POOL_SIZE);
        mVirtioGpuOps->create_yuv_textures(kFRAMEWORK_FORMAT_NV12,
                                           kFRAME_POOL_SIZE, w, h,
                                           textures.data());
        for (uint32_t i = 0; i < kFRAME_POOL_SIZE; ++i) {
            TextureFrame frame{textures[2 * i], textures[2 * i + 1]};
            H264_DPRINT("allocated Y %d UV %d", frame.Ytex, frame.UVtex);
            m_Frame_to_PoolHandle[TexFrame{frame.Ytex, frame.UVtex}] = ph;
            ph->push_back(frame);
        }
    }
    TextureFrame frame = ph->front();
    ph->pop_front();
    H264_DPRINT("done %s %d ret Y %d UV %d", __func__, __LINE__, frame.Ytex,
                frame.UVtex);
    return frame;
}

void MediaTexturePool::saveDecodedFrameToTexture(TextureFrame frame,
                                                 void* privData,
                                                 void* func) {
    H264_DPRINT("calling %s %d for tex of  %d  %d\n", __func__, __LINE__,
                (int)frame.Ytex, (int)frame.UVtex);
    if (mVirtioGpuOps) {
        uint32_t textures[2] = {frame.Ytex, frame.UVtex};
        mVirtioGpuOps->update_yuv_textures(kFRAMEWORK_FORMAT_NV12, textures,
                                           privData, func);
    }
}

void MediaTexturePool::putTextureFrame(TextureFrame frame) {
    H264_DPRINT("try recycle textures %d %d", (int)frame.Ytex,
                (int)frame.UVtex);
    if (frame.Ytex > 0 && frame.UVtex > 0) {
        TexFrame tframe{frame.Ytex, frame.UVtex};
        auto iter = m_Frame_to_PoolHandle.find(tframe);
        if (iter != m_Frame_to_PoolHandle.end()) {
            PoolHandle phandle = iter->second;
            H264_DPRINT("recycle registered textures %d %d", (int)frame.Ytex,
                        (int)frame.UVtex);
            phandle->push_back(std::move(frame));
        } else {
            H264_DPRINT("recycle un-registered textures %d %d", (int)frame.Ytex,
                        (int)frame.UVtex);
            deleteTextures(frame);
        }
    }
}

void MediaTexturePool::deleteTextures(TextureFrame frame) {
    if (mVirtioGpuOps && frame.Ytex > 0 && frame.UVtex > 0) {
        std::vector<uint32_t> textures;
        textures.push_back(frame.Ytex);
        textures.push_back(frame.UVtex);
        mVirtioGpuOps->destroy_yuv_textures(kFRAMEWORK_FORMAT_NV12, 1,
                                            textures.data());
    }
}

void MediaTexturePool::cleanUpTextures() {
    if (m_WH_to_PoolHandle.empty()) {
        return;
    }
    for (auto iter : m_WH_to_PoolHandle) {
        auto& myFramePool = *(iter.second);
        std::vector<uint32_t> textures;
        for (auto& frame : myFramePool) {
            textures.push_back(frame.Ytex);
            textures.push_back(frame.UVtex);
            H264_DPRINT("delete Y %d UV %d", frame.Ytex, frame.UVtex);
        }
        mVirtioGpuOps->destroy_yuv_textures(
                kFRAMEWORK_FORMAT_NV12, myFramePool.size(), textures.data());
        myFramePool.clear();
    }
}

}  // namespace emulation
}  // namespace android
