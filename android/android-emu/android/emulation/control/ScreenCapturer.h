// Copyright (C) 2016 The Android Open Source Project
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

#include <stddef.h>                   // for NULL
#include <cstdint>                    // for uint8_t, uint16_t, uint32_t
#include <functional>                 // for function
#include <string>                     // for string
#include <string_view>
#include <utility>                    // for move
#include <vector>                     // for vector

#include "aemu/base/export.h"
#include "android/skin/rect.h"        // for SkinRotation

namespace gfxstream {
class Renderer;
}

namespace android {
namespace emulation {

enum class ImageFormat { PNG, RAW, RGB888, RGBA8888 };

class Image {
public:
    AEMU_EXPORT Image(uint16_t w,
          uint16_t h,
          uint8_t channels,
          ImageFormat format,
          std::vector<uint8_t> pixels)
        : m_Width(w),
          m_Height(h),
          m_NChannels(channels),
          m_Format(format),
          m_Pixels(std::move(pixels)) {}

    AEMU_EXPORT uint8_t* getPixelBuf() { return m_Pixels.data(); }
    AEMU_EXPORT uint64_t getPixelCount() { return m_Pixels.size(); }
    AEMU_EXPORT uint16_t getWidth() { return m_Width; }
    AEMU_EXPORT uint16_t getHeight() { return m_Height; }
    AEMU_EXPORT uint8_t getChannels() { return m_NChannels; }
    AEMU_EXPORT ImageFormat getImageFormat() { return m_Format; }

    // Converts Image from RGBA8888 -> RGB888 if needed and possible.
    AEMU_EXPORT Image& asRGB888();

private:
    void convertPerByte();
    void convertPerHexlet();
    uint16_t m_Width;
    uint16_t m_Height;
    uint8_t m_NChannels;
    ImageFormat m_Format;
    std::vector<uint8_t> m_Pixels;
};

// Capture a screenshot using the Render (if set) or framebuffer callback.
// if desiredWidth and desiredHeight are 0, the wide and height of the
// screen colorbuffer will be used.
AEMU_EXPORT Image takeScreenshot(
        ImageFormat desiredFormat,
        SkinRotation rotation,
        gfxstream::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        int displayId = 0,
        int desiredWidth = 0,
        int desiredHeight = 0,
        SkinRect rect = {{0, 0}, {0, 0}});

AEMU_EXPORT bool captureScreenshot(const char* outputDirectoryPath,
                       std::string* outputFilepath = NULL,
                       uint32_t displayId = 0);
// The following one is for testing only
// It loads texture from renderer if renderer is not null.
// (-gpu host, swiftshader_indirect, angle_indirect)
// Otherwise loads texture from getFrameBuffer function. (-gpu guest)
AEMU_EXPORT bool captureScreenshot(
        gfxstream::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        SkinRotation rotation,
        const char* outputDirectoryPath,
        std::string* outputFilepath = nullptr,
        int displayId = 0);

}  // namespace emulation
}  // namespace android
