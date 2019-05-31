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
#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaCodec.h"
#include <inttypes.h>

extern "C" {
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vpx_image.h"
#include "vpx/vp8dx.h"
}

namespace android {
namespace emulation {

class MediaVpxDecoder : MediaCodec {
public:
    MediaVpxDecoder() = default;
    virtual ~MediaVpxDecoder() = default;

public:
    // this is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;
    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

private:
    void initVpxContext(void *ptr, MediaCodecType type);
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
