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

#include "android/base/files/Stream.h"
#include "android/emulation/GoldfishMediaDefs.h"

namespace android {
namespace emulation {

class MediaCodec {
public:
    MediaCodec() = default;
    virtual ~MediaCodec() = default;

    // Handler called from the guest media device.
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) = 0;

    // For snapshots
    virtual void save(base::Stream* stream) const = 0;
    virtual bool load(base::Stream* stream) = 0;
};

}  // namespace emulation
}  // namespace android
