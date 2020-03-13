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

namespace android {
namespace emulation {

// This is a helper class to render decoded frames
// to host color buffer
class MediaHostRenderer {
public:
    // for now, there is only NV12
    struct TextureFrame {
        uint32_t Ytex;
        uint32_t UVtex;
    };

    // get a TextureFrame structure to hold decoded frame
    TextureFrame getTextureFrame(int w, int h);

private:
    // put back a used TextureFrame so it can be reused again
    // later
    void putTextureFrame(TextureFrame frame) {
        mFramePool.push_back(std::move(frame));
    }

    void cleanUpTextures();

    std::list<TextureFrame> mFramePool;

public:
    // render decoded frame stored in CPU memory
    void renderToHostColorBuffer(int hostColorBufferId,
                                 unsigned int outputWidth,
                                 unsigned int outputHeight,
                                 uint8_t* decodedFrame);

    // render decoded frame stored in GPU texture; recycle the swapped
    // out texture from colorbuffer into framepool
    void renderToHostColorBufferWithTextures(int hostColorBufferId,
                                             unsigned int outputWidth,
                                             unsigned int outputHeight,
                                             TextureFrame frame);

    MediaHostRenderer();
    ~MediaHostRenderer();

private:
    AndroidVirtioGpuOps* mVirtioGpuOps = nullptr;
};

}  // namespace emulation
}  // namespace android
