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

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaVideoHelper {
public:
    MediaVideoHelper() = default;
    virtual ~MediaVideoHelper() = default;
    // return true if success; false otherwise
    virtual bool init() { return false; }
    virtual void decode(const uint8_t* frame,
                        size_t szBytes,
                        uint64_t inputPts) {}
    virtual void flush() {}
    virtual void deInit() {}

    bool receiveFrame(MediaSnapshotState::FrameInfo* pFrameInfo);
    void setIgnoreDecodedFrames() { mIgnoreDecoderOutput = true; }
    void setSaveDecodedFrames() { mIgnoreDecoderOutput = false; }

    virtual int error() const { return 0; }
    virtual bool good() const { return true; }
    virtual bool fatal() const { return false; }

protected:
    bool mIgnoreDecoderOutput = false;

    mutable std::list<MediaSnapshotState::FrameInfo> mSavedDecodedFrames;

};  // MediaVideoHelper

}  // namespace emulation
}  // namespace android
