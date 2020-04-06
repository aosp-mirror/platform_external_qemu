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

#include "android/base/files/Stream.h"
#include "android/emulation/MediaSnapshotState.h"

#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaSnapshotHelper {
public:
    enum class CodecType {
        H264 = 1,
        VP8 = 2,
        VP9 = 3,
    };

public:
    MediaSnapshotHelper(CodecType type) : mType(type) {}
    ~MediaSnapshotHelper() = default;

public:
public:
    void savePacket(const uint8_t* compressedFrame, size_t len, uint64_t pts);

    void save(base::Stream* stream) const;

    void saveDecodedFrame(std::vector<uint8_t> data,
                          int width = 0,
                          int height = 0,
                          uint64_t pts = 0,
                          MediaSnapshotState::ColorAspects xcolor =
                                  MediaSnapshotState::ColorAspects{}) {
        mSnapshotState.saveDecodedFrame(std::move(data), width, height, pts,
                                        xcolor);
    }

    void saveDecodedFrame(MediaSnapshotState::FrameInfo frame) {
        mSnapshotState.saveDecodedFrame(std::move(frame));
    }

    void saveDecodedFrame(std::vector<uint32_t> texture,
                          int width = 0,
                          int height = 0,
                          uint64_t pts = 0,
                          MediaSnapshotState::ColorAspects xcolor =
                                  MediaSnapshotState::ColorAspects{}) {
        mSnapshotState.saveDecodedFrame(texture, width, height, pts, xcolor);
    }

    MediaSnapshotState::FrameInfo* frontFrame() {
        return mSnapshotState.frontFrame();
    }

    void discardFrontFrame() { mSnapshotState.discardFrontFrame(); }

    void replay(std::function<void(uint8_t* data, size_t len, uint64_t pts)>
                        oneShotDecode);

    void load(base::Stream* stream,
              std::function<void(uint8_t* data, size_t len, uint64_t pts)>
                      oneShotDecode);

private:
    CodecType mType = CodecType::H264;
    mutable MediaSnapshotState mSnapshotState;

    void saveVPXPacket(const uint8_t* compressedFrame,
                       size_t len,
                       uint64_t pts);
    void saveH264Packet(const uint8_t* compressedFrame,
                        size_t len,
                        uint64_t pts);

private:
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = 0;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    unsigned int mOutBufferSize = 0;

    unsigned int mColorPrimaries = 2;
    unsigned int mColorRange = 0;
    unsigned int mColorTransfer = 2;
    unsigned int mColorSpace = 2;

};  // MediaSnapshotHelper

}  // namespace emulation
}  // namespace android
