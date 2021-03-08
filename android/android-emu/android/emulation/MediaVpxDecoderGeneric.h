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
#include "android/emulation/MediaSnapshotHelper.h"
#include "android/emulation/MediaSnapshotState.h"
#include "android/emulation/MediaVideoHelper.h"
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

class MediaVpxDecoderGeneric : public MediaVpxDecoderPlugin {
public:
    virtual void initVpxContext(void* ptr) override;
    virtual void decodeFrame(void* ptr) override;
    virtual void getImage(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void destroyVpxContext(void* ptr) override;

    explicit MediaVpxDecoderGeneric(VpxPingInfoParser parser,
                                    MediaCodecType type);
    virtual ~MediaVpxDecoderGeneric();

    virtual void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

    friend MediaSnapshotHelper;

protected:
    // this is required by save/load
    virtual int type() const override { return PLUGIN_TYPE_GENERIC; }
    virtual int vpxtype() const override {
        return mType == MediaCodecType::VP8Codec ? 8 : 9;
    }
    virtual int version() const override { return mParser.version(); }

private:
    MediaCodecType mType;  // vp8 or vp9
    VpxPingInfoParser mParser;
    MediaHostRenderer mRenderer;

    int mNumFramesDecoded = 0;

    bool mUseGpuTexture = false;
    bool mTrialPeriod = true;
    // at any point of time, only one of the following is valid
    std::unique_ptr<MediaVideoHelper> mHwVideoHelper;
    std::unique_ptr<MediaVideoHelper> mSwVideoHelper;
    std::unique_ptr<MediaVideoHelper> mVideoHelper;

    void fetchAllFrames();
    void createAndInitSoftVideoHelper();

private:
    void decode_internal(const uint8_t* data, size_t len, uint64_t pts);
    void oneShotDecode(const uint8_t* data, size_t len, uint64_t pts);
    void try_decode(const uint8_t* data, size_t len, uint64_t pts);
    mutable MediaSnapshotHelper mSnapshotHelper;
};

}  // namespace emulation
}  // namespace android
