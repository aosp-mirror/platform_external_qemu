// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

///
// VP9CodecHelper.h
//
// Helper to configure VP9 encoder
//

#pragma once

#include "android/recording/codecs/CodecHelper.h"

extern "C" {
#include "libswscale/swscale.h"
}

namespace android {
namespace recording {

class VP9CodecHelper : public CodecHelper<SwsContext*> {
public:
    explicit VP9CodecHelper(CodecParams&& params,
                            uint32_t mFbWidth,
                            uint32_t mFbHeight,
                            AVPixelFormat fbFormat);
    virtual ~VP9CodecHelper();

    // Configures the encoder. Returns true if successful, false otherwise.
    virtual bool configAndOpenEncoder(const AVFormatContext* oc,
                                      AVStream* stream) const override;
    // Configures and initializes the rescaling context.
    virtual bool initSwxContext(const AVCodecContext* c,
                                SwsContext** swsCxt) const override;

protected:
    uint32_t mFbWidth = 0;
    uint32_t mFbHeight = 0;
    AVPixelFormat mFbFormat = AV_PIX_FMT_NONE;
};

}  // namespace recording
}  // namespace android
