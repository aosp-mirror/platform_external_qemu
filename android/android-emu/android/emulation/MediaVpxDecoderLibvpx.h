// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/MediaHostRenderer.h"
#include "android/emulation/MediaVpxDecoderPlugin.h"
#include "android/emulation/VpxPingInfoParser.h"

#include <stddef.h>
#include <stdint.h>
#include <vector>

extern "C" {
#include "vpx/vp8dx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vpx_image.h"
}

namespace android {
namespace emulation {

class MediaVpxDecoderLibvpx : public MediaVpxDecoderPlugin {
public:
    virtual void initVpxContext(void* ptr) override;
    virtual void decodeFrame(void* ptr) override;
    virtual void getImage(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void destroyVpxContext(void* ptr) override;

    explicit MediaVpxDecoderLibvpx(VpxPingInfoParser parser,
                                   MediaCodecType type);
    virtual ~MediaVpxDecoderLibvpx();

protected:
    // this is required by save/load
    virtual int type() const override { return PLUGIN_TYPE_LIBVPX; }

private:
    MediaCodecType mType;  // vp8 or vp9
    VpxPingInfoParser mParser;
    MediaHostRenderer mRenderer;

    std::unique_ptr<vpx_codec_ctx_t> mCtx;

    bool mImageReady = false;

    // Owned by the vpx context. Needs to be initialized to nullptr on the first
    // vpx_codec_get_frame() call.
    vpx_codec_iter_t mIter = nullptr;

    // Owned by the vpx context. mImg is set when calling vpx_codec_get_frame().
    // mImg remains valid until the next vpx_codec_decode() is called.
    vpx_image_t* mImg = nullptr;

    // helper methods
    void copyImgToGuest(vpx_image_t* mImg, const GetImageParam& param);
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
};

}  // namespace emulation
}  // namespace android
