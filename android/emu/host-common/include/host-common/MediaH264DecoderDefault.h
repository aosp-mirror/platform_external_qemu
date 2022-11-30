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
#include "android/emulation/MediaH264DecoderPlugin.h"
#include "android/emulation/MediaH264Decoder.h"

#include <stddef.h>
#include <mutex>
#include <unordered_map>

namespace android {
namespace emulation {


class MediaH264DecoderDefault : public MediaH264Decoder {
public:
    MediaH264DecoderDefault() = default;
    virtual ~MediaH264DecoderDefault() = default;

    // This is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

    // For snapshots
    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

private:
    std::mutex mIdLock{};
    std::mutex mMapLock{};
    uint64_t mId = 0;
    std::unordered_map<uint64_t, MediaH264DecoderPlugin*> mDecoders;
    uint64_t readId(void* ptr);  // read id from the address
    void removeDecoder(uint64_t id);
    uint64_t createId();
    void addDecoder(uint64_t key,
                    MediaH264DecoderPlugin* val);  // this just add
    void updateDecoder(
            uint64_t key,
            MediaH264DecoderPlugin* val);  // this will overwrite
    MediaH264DecoderPlugin* getDecoder(uint64_t key);
};

}  // namespace emulation
}  // namespace android
