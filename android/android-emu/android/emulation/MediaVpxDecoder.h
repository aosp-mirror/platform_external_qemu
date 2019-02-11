// Copyright 2019 The Android Open Source Project
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

#include <inttypes.h>

extern "C" {
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vpx_image.h"
#include "vpx/vp8dx.h"
}

namespace android {
namespace emulation {

class MediaVpxDecoder {
public:
    MediaVpxDecoder() = default;
    ~MediaVpxDecoder() = default;

public:

    // this is the entry point
    void handlePing(uint64_t metadata, void* ptr);

private:

    void initVpxContext(void *ptr, int vpversion);
    void destroyVpxContext(void *ptr);
    void decodeFrame(void* ptr);
    void flush(void* ptr);
    void getImage(void* ptr);

private:

    vpx_codec_ctx_t *mCtx = NULL;
    bool mImageReady = false;
    vpx_codec_iter_t  mIter = NULL;
    vpx_image_t *mImg = NULL;
};

}  // namespace emulation
}  // namespace android
