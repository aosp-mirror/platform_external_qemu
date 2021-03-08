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

// A parser for VpxFrame
// the specified data.

#pragma once

#include <cstdint>
#include <string>

namespace android {
namespace emulation {

class VpxFrameParser {
public:
    VpxFrameParser(int vpxtype, const uint8_t* data, size_t size);
    ~VpxFrameParser() = default;

public:
    bool isKeyFrame() const { return m_is_key_frame; }

private:
    int type = 0;  // 8: vp8, 9: vp9
    const uint8_t* bit_buffer = nullptr;
    const uint8_t* bit_buffer_end = nullptr;
    size_t bit_offset = 0;

    bool m_is_key_frame = false;

private:
    static constexpr int KEY_FRAME = 0;
    static constexpr int INTER_FRAME = 1;

    int read_bit();
    int read_literal(int bits);

    void parse_vp9_uncompressed_header();
    void parse_vp8_uncompressed_header();
};

}  // namespace emulation
}  // namespace android
