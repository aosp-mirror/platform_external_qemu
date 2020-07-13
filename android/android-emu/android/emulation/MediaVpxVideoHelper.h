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

#include "android/emulation/MediaSnapshotState.h"
#include "android/emulation/MediaVideoHelper.h"

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include "vpx/vp8dx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vpx_image.h"
}

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaVpxVideoHelper : public MediaVideoHelper {
public:
    MediaVpxVideoHelper(int type, int threads);
    ~MediaVpxVideoHelper() override;

    // return true if success; false otherwise
    bool init() override;
    void decode(const uint8_t* frame,
                size_t szBytes,
                uint64_t inputPts) override;
    void flush() override;
    void deInit() override;

private:
    int mType = 0;
    int mThreadCount = 1;

    // vpx stuff
    std::unique_ptr<vpx_codec_ctx_t> mCtx;

    // Owned by the vpx context. Needs to be initialized to nullptr on the first
    // vpx_codec_get_frame() call.
    vpx_codec_iter_t mIter = nullptr;

    // Owned by the vpx context. mImg is set when calling vpx_codec_get_frame().
    // mImg remains valid until the next vpx_codec_decode() is called.
    vpx_image_t* mImg = nullptr;

    void fetchAllFrames();
    // helper methods
    void copyImgToGuest(vpx_image_t* mImg, std::vector<uint8_t>& byteBuffer);
    void copyYV12FrameToOutputBuffer(size_t outputBufferWidth,
                                     size_t outputBufferHeight,
                                     size_t imgWidth,
                                     size_t imgHeight,
                                     int32_t bpp,
                                     uint8_t* dst,
                                     const uint8_t* srcY,
                                     const uint8_t* srcU,
                                     const uint8_t* srcV,
                                     size_t srcYStride,
                                     size_t srcUStride,
                                     size_t srcVStride);

};  // MediaVpxVideoHelper

}  // namespace emulation
}  // namespace android
