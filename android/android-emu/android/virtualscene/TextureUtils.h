/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*
 * Defines TextureUtils, which provides helpers loading texture assets from
 * file.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"

#include <vector>

namespace android {
namespace virtualscene {

class TextureUtils {
public:
    enum class Format {
        RGB24,   // Packed, 3-bytes-per-pixel, with stride aligned to a 4-byte
                 // boundary.  Matches what glTexImage2D with GL_RGB and
                 // GL_UNSIGNED_BYTE expects with GL_UNPACK_ALIGNMENT==4.
        RGBA32,  // Matches GL_RGBA.
    };

    // Loads a PNG from disk, converting to either RGB or RGBA if necessary.
    //
    // The image is oriented bottom-up to match glTexImage2D's data layout.
    //
    // |filename| - Filename to load.
    // |buffer| - Output buffer, existing data is replaced.
    // |outWidth| - Output width, in pixels.
    // |outHeight| - Output height, in pixels.
    // |outFormat| - Output format.
    static bool loadPNG(const char* filename,
                        std::vector<uint8_t>& buffer,
                        uint32_t* outWidth,
                        uint32_t* outHeight,
                        Format* outFormat);
};

}  // namespace virtualscene
}  // namespace android
