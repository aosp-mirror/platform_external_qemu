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

#include <stdint.h>

// We need to keep this in sync with guest side code
// (device/generic/goldfish/codecs/goldfish_common/goldfish_media_utils.h).
// Probably should move this part into goldfish-opengl.
enum class MediaCodecType : uint8_t {
    VP8Codec = 0,
    VP9Codec = 1,
    H264Codec = 2,
    Max = 3,
};

enum class MediaOperation : uint8_t {
    InitContext = 0,
    DestroyContext = 1,
    DecodeImage = 2,
    GetImage = 3,
    Flush = 4,
    Reset = 5,
    Max = 6,
};
