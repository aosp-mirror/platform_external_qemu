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
#include "android/base/Optional.h"
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

    enum class Orientation {
        TopDown,
        BottomUp,
        Qt = TopDown,
        OpenGL = BottomUp,  // glTexImage2D's data layout's orientation is
                            // bottom up by default
    };

    struct Result {
        // Pixel data.
        std::vector<uint8_t> mBuffer;
        // Width, in pixels.
        uint32_t mWidth = 0;
        // Height, in pixels.
        uint32_t mHeight = 0;
        // Pixel format.
        Format mFormat = Format::RGB24;
    };

    // Create an empty texture with a given size.
    static Result createEmpty(uint32_t width, uint32_t height);

    // Create a placeholder texture, which is 1x1 and transparent, to be used
    // for asynchronous texture loading.
    static Result createPlaceholder();

    // Loads an image from disk, converting to either RGB or RGBA if necessary.
    // The format loaded is determined by the file's extension.
    //
    // The image is oriented bottom-up by default to match glTexImage2D's data
    // layout.
    //
    // |filename| - Filename to load.
    // |orientation| - Image orientation, match OpenGL by default.
    //
    // If the load was successful, returns a Result.
    static android::base::Optional<Result> load(
            const char* filename,
            Orientation orientation = Orientation::OpenGL);

    // Loads a PNG from disk, converting to either RGB or RGBA if necessary.
    //
    // The image is oriented bottom-up by default to match glTexImage2D's data
    // layout.
    //
    // |filename| - Filename to load.
    // |orientation| - Image orientation, match OpenGL by default.
    //
    // If the load was successful, returns a Result.
    static android::base::Optional<Result> loadPNG(
            const char* filename,
            Orientation orientation = Orientation::OpenGL);

    // Loads a JPEG from disk.
    //
    // The image is oriented bottom-up by default to match glTexImage2D's data
    // layout.
    //
    // |filename| - Filename to load.
    // |orientation| - Image orientation, match OpenGL by default.
    //
    // If the load was successful, returns a Result.
    static android::base::Optional<Result> loadJPEG(
            const char* filename,
            Orientation orientation = Orientation::OpenGL);
};

}  // namespace virtualscene
}  // namespace android
