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
#include "android/emulation/MediaTexturePool.h"
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
    using TextureFrame = MediaTexturePool::TextureFrame;
    // get a TextureFrame structure to hold decoded frame
    MediaTexturePool::TextureFrame getTextureFrame(int w, int h);

    void saveDecodedFrameToTexture(MediaTexturePool::TextureFrame frame,
                                   void* privData,
                                   void* func);

    // put back a used TextureFrame so it can be reused again
    // later
    void putTextureFrame(MediaTexturePool::TextureFrame frame) {
        mTexturePool.putTextureFrame(frame);
    }

    void cleanUpTextures();

    MediaTexturePool* getTexturePool() { return &mTexturePool; }

private:
    MediaTexturePool mTexturePool;

public:
    // render decoded frame stored in CPU memory
    void renderToHostColorBuffer(int hostColorBufferId,
                                 unsigned int outputWidth,
                                 unsigned int outputHeight,
                                 uint8_t* decodedFrame);

    // render decoded frame stored in GPU texture; recycle the swapped
    // out texture from colorbuffer into framepool
    void renderToHostColorBufferWithTextures(
            int hostColorBufferId,
            unsigned int outputWidth,
            unsigned int outputHeight,
            MediaTexturePool::TextureFrame frame);

    MediaHostRenderer();
    ~MediaHostRenderer();

private:
    AndroidVirtioGpuOps* mVirtioGpuOps = nullptr;
};

}  // namespace emulation
}  // namespace android
