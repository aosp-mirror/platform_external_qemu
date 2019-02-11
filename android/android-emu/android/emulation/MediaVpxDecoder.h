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

#include <memory>
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

    enum class VpxVersion: uint8_t {
        VP8 = 8,
        VP9 = 9,
    };

public:
    // this is the entry point
    void handlePing(uint64_t metadata, void* ptr);

private:
    void initVpxContext(void *ptr, VpxVersion version);
    void destroyVpxContext(void *ptr);
    void decodeFrame(void* ptr);
    void flush(void* ptr);
    void getImage(void* ptr);

private:
    std::unique_ptr<vpx_codec_ctx_t> mCtx;
    bool mImageReady = false;
    // Owned by the vpx context. Needs to be initialized to nullptr on the first
    // vpx_codec_get_frame() call.
    vpx_codec_iter_t mIter = nullptr;
    // Owned by the vpx context. mImg is set when calling vpx_codec_get_frame(). mImg remains
    // valid until the next vpx_codec_decode() is called.
    vpx_image_t* mImg = nullptr;
};

}  // namespace emulation
}  // namespace android
