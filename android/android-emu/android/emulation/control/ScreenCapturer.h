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
#include <utility>                    // for move
#include <vector>                     // for vector

#include "android/base/StringView.h"  // for StringView
#include "android/skin/rect.h"        // for SkinRotation

namespace emugl {
class Renderer;
}

namespace android {
namespace emulation {

enum class ImageFormat { PNG, RAW, RGB888, RGBA8888 };

class Image {
public:
    Image(uint16_t w,
          uint16_t h,
          uint8_t channels,
          ImageFormat format,
          std::vector<uint8_t> pixels)
        : m_Width(w),
          m_Height(h),
          m_NChannels(channels),
          m_Format(format),
          m_Pixels(std::move(pixels)) {}

    uint8_t* getPixelBuf() { return m_Pixels.data(); }
    uint64_t getPixelCount() { return m_Pixels.size(); }
    uint16_t getWidth() { return m_Width; }
    uint16_t getHeight() { return m_Height; }
    uint8_t getChannels() { return m_NChannels; }
    ImageFormat getImageFormat() { return m_Format; }

    // Converts Image from RGBA8888 -> RGB888 if needed and possible.
    Image& asRGB888();

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
Image takeScreenshot(
        ImageFormat desiredFormat,
        SkinRotation rotation,
        emugl::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        int displayId = 0,
        int desiredWidth = 0,
        int desiredHeight = 0
        );

bool captureScreenshot(android::base::StringView outputDirectoryPath,
                       std::string* outputFilepath = NULL,
                       uint32_t displayId = 0);
// The following one is for testing only
// It loads texture from renderer if renderer is not null.
// (-gpu host, swiftshader_indirect, angle_indirect)
// Otherwise loads texture from getFrameBuffer function. (-gpu guest)
bool captureScreenshot(
        emugl::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        SkinRotation rotation,
        android::base::StringView outputDirectoryPath,
        std::string* outputFilepath = nullptr,
        int displayId = 0);

}  // namespace emulation
}  // namespace android
