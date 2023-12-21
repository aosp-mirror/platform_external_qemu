// Copyright (C) 2022 The Android Open Source Project
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

#include "host-common/GoldfishMediaDefs.h"
#include "android/emulation/HevcPingInfoParser.h"
#include "host-common/MediaFfmpegVideoHelper.h"
#include "android/emulation/MediaHevcDecoderPlugin.h"
#include "host-common/MediaHostRenderer.h"
#include "host-common/MediaSnapshotHelper.h"
#include "host-common/MediaSnapshotState.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaHevcDecoderGeneric : public MediaHevcDecoderPlugin {
public:
    virtual void reset(void* ptr) override;
    virtual void initHevcContext(void* ptr) override;
    virtual MediaHevcDecoderPlugin* clone() override;
    virtual void destroyHevcContext() override;
    virtual void decodeFrame(void* ptr) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;
    virtual void sendMetadata(void* ptr) override;

    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

    explicit MediaHevcDecoderGeneric(uint64_t id, HevcPingInfoParser parser);
    virtual ~MediaHevcDecoderGeneric();

    virtual int type() const override { return PLUGIN_TYPE_GENERIC; }

    friend MediaHevcDecoderDefault;

public:
private:
    void decodeFrameInternal(const uint8_t* data, size_t len, uint64_t pts);

    void try_decode(const uint8_t* data, size_t len, uint64_t pts);

    void initHevcContextInternal(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt);
    uint64_t mId = 0;
    HevcPingInfoParser mParser;
    MediaHostRenderer mRenderer;
    // image props
    unsigned int mWidth = 0;
    unsigned int mHeight = 0;
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    uint64_t mOutputPts = 0;
    uint64_t mInputPts = 0;
    PixelFormat mOutPixFmt;

    // color aspects related
private:
    // default is 601, limited range, sRGB
    std::unique_ptr<MetadataParam> mMetadataPtr;

private:
    std::unique_ptr<MediaSnapshotHelper> mSnapshotHelper;
    bool mUseGpuTexture = false;

    // at any point of time, only one of the following is valid
    std::unique_ptr<MediaVideoHelper> mHwVideoHelper;
    std::unique_ptr<MediaVideoHelper> mSwVideoHelper;
    std::unique_ptr<MediaVideoHelper> mVideoHelper;

    bool mTrialPeriod = true;

private:
    void fetchAllFrames();

    void createAndInitSoftVideoHelper();

    void oneShotDecode(const uint8_t* data, size_t len, uint64_t pts);
};  // MediaHevcDecoderGeneric

}  // namespace emulation
}  // namespace android
