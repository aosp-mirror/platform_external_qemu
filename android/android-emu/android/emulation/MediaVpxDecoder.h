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

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaCodec.h"
#include "android/emulation/MediaVpxDecoderPlugin.h"
#include "android/emulation/VpxPingInfoParser.h"

#include <inttypes.h>
#include <stddef.h>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace android {
namespace emulation {

class MediaVpxDecoder : MediaCodec {
public:
    using InitContextParam = VpxPingInfoParser::InitContextParam;
    using DecodeFrameParam = VpxPingInfoParser::DecodeFrameParam;
    using GetImageParam = VpxPingInfoParser::GetImageParam;

    MediaVpxDecoder() = default;
    virtual ~MediaVpxDecoder() = default;
    void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

public:
    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;

public:
    void initVpxContext(void *ptr, MediaCodecType type);
    void destroyVpxContext(void *ptr);
    void decodeFrame(void* ptr);
    void flush(void* ptr);
    void getImage(void* ptr);

private:
    std::mutex mMapLock{};
    uint64_t mId = 0;
    std::unordered_map<uint64_t, MediaVpxDecoderPlugin*> mDecoders;
    uint64_t readId(void* ptr);  // read id from the address
    void removeDecoder(uint64_t id);
    void addDecoder(uint64_t key,
                    MediaVpxDecoderPlugin* val);  // this just add
    void updateDecoder(uint64_t key,
                       MediaVpxDecoderPlugin* val);  // this will overwrite
    MediaVpxDecoderPlugin* getDecoder(uint64_t key);
};

}  // namespace emulation
}  // namespace android
