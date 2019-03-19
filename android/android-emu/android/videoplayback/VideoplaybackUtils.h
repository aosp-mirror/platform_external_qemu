/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/base/Optional.h"

namespace android {
namespace videoplayback {

// Struct containing all the metadata in a PPM image header.
struct PPMInfo {
    int width;
    int height;

    // The largest possible value a color can be in the texel data.
    int maxColor;
};

// Parses data and fills out info with relevant metadata from the PPM header.
// Returns an index to the start of the image texels if successful, or -1 on any
// failure.
base::Optional<size_t> parsePPMHeader(PPMInfo* info,
                                      const unsigned char* data,
                                      size_t dataLen);

}  // namespace videoplayback
}  // namespace android
