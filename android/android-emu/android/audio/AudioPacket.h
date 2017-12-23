// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/system/System.h"

#include <memory>

namespace android {
namespace audio {

// A small structure to encapsulate the audio data to
// pass between the main loop thread and another thread.
struct AudioPacket {
    uint64_t tsUs = 0llu;  // timestamp(microsec)
    uint32_t size = 0;
    uint32_t capacity = 0;
    std::unique_ptr<uint8_t> data;

    // Need default constructor for std::vector
    AudioPacket() {}

    // For allocating the space but with no data
    AudioPacket(uint32_t cap) : capacity(cap) {
        data.reset(new uint8_t[capacity]);
    }

    // For allocating just enough space for the data
    AudioPacket(void* buf, uint32_t size_bytes)
        : size(size_bytes), capacity(size_bytes) {
        if (buf != nullptr && size > 0) {
            tsUs = android::base::System::get()->getHighResTimeUs();
            data.reset(new uint8_t[size]);
            ::memcpy(static_cast<void*>(data.get()), buf, size);
        }
    }
};

}  // namespace audio
}  // namespace android
