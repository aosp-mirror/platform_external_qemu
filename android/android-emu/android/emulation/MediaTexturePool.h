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

#pragma once

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/opengles.h"

#include <stddef.h>
#include <stdint.h>
#include <list>
#include <map>

namespace android {
namespace emulation {

// This is a helper class to render decoded frames
// to host color buffer
class MediaTexturePool {
public:
    // for now, there is only NV12
    struct TextureFrame {
        uint32_t Ytex;
        uint32_t UVtex;
    };

    // get a TextureFrame structure to hold decoded frame
    TextureFrame getTextureFrame(int w, int h);

    void saveDecodedFrameToTexture(TextureFrame frame,
                                   void* privData,
                                   void* func);

    // put back a used TextureFrame so it can be reused again
    // later
    void putTextureFrame(TextureFrame frame);

    void cleanUpTextures();

private:
    using TexSizes = std::pair<int, int>;
    using TexFrame = std::pair<uint32_t, uint32_t>;
    using Pool = std::list<TextureFrame>;
    using PoolHandle = std::list<TextureFrame>*;
    std::map<TexSizes, PoolHandle> m_WH_to_PoolHandle;
    std::map<TexFrame, TexSizes> m_Frame_to_WH;
    std::map<TexFrame, PoolHandle> m_Frame_to_PoolHandle;

    void deleteTextures(TextureFrame frame);

    int m_id = 0;

public:
    MediaTexturePool();
    ~MediaTexturePool();

private:
    AndroidVirtioGpuOps* mVirtioGpuOps = nullptr;
};

}  // namespace emulation
}  // namespace android
