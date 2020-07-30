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

#include "android/emulation/control/ScreenCapturer.h"

#include <assert.h>                                   // for assert
#include <png.h>                                      // for png_create_info...
#include <stdio.h>                                    // for NULL, snprintf
#include <string.h>                                   // for memcpy
#include <fstream>                                    // for ofstream, basic...
#include <memory>                                     // for shared_ptr
#include <vector>                                     // for vector

#include "OpenglRender/Renderer.h"                    // for Renderer
#include "android/base/Log.h"                         // for LOG, LogMessage
#include "android/base/files/PathUtils.h"             // for PathUtils
#include "android/base/system/System.h"               // for System
#include "android/console.h"                          // for getConsoleAgents
#include "android/emulation/control/display_agent.h"  // for QAndroidDisplay...
#include "android/emulation/control/window_agent.h"   // for QAndroidEmulato...
#include "android/emulator-window.h"                  // for emulator_window...
#include "android/loadpng.h"                          // for savepng, write_...
#include "android/opengles.h"                         // for android_getOpen...
#include "android/utils/string.h"                     // for str_ends_with
#include "observation.pb.h"                           // for Observation
#include "pngconf.h"                                  // for png_byte, png_b...

namespace android {
namespace emulation {

bool captureScreenshot(android::base::StringView outputDirectoryPath,
                       std::string* pOutputFilepath,
                       uint32_t displayId) {
    const auto& renderer = android_getOpenglesRenderer();
    SkinRotation rotation = getConsoleAgents()->emu->getRotation();
    if (const auto renderer_ptr = renderer.get()) {
        return captureScreenshot(renderer_ptr, nullptr, rotation,
                                 outputDirectoryPath, pOutputFilepath, displayId);
    } else {
        // renderer is nullptr in -gpu guest
        if (displayId > 0) {
            return false;
        }
        return captureScreenshot(
                nullptr,
                emulator_window_get()->uiEmuAgent->display->getFrameBuffer,
                rotation, outputDirectoryPath, pOutputFilepath);
    }
}

Image takeScreenshot(
        ImageFormat desiredFormat,
        SkinRotation rotation,
        emugl::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        int displayId,
        int desiredWidth,
        int desiredHeight
        ) {
    unsigned int nChannels = 4;
    unsigned int width;
    unsigned int height;
    ImageFormat outputFormat = ImageFormat::RGBA8888;
    std::vector<unsigned char> pixelBuffer;
    if (renderer) {
        renderer->getScreenshot(nChannels, &width, &height, pixelBuffer, displayId,
                                desiredWidth, desiredHeight, rotation);
    } else {
        unsigned char* pixels = nullptr;
        int bpp = 4;
        int lineSize = 0;
        getFrameBuffer((int*)&width, (int*)&height, &lineSize, &bpp, &pixels);
        if (bpp < 1 || bpp > 4) {
            // unknown pixel buffer format
            return Image(0, 0, 0, ImageFormat::RGB888, {});
        }
        // -gpu guest usually gives us bpp=2
        // bpp=2 infers we are using rgb565
        // convert it to rgb888
        nChannels = bpp == 2 ? 3 : bpp;
        outputFormat = ImageFormat::RGB888;
        // Need to handle padding if lineSize != width * nChannels
        if ((lineSize != 0 && lineSize != width * nChannels) || bpp == 2) {
            pixelBuffer.resize(width * height * nChannels);
            unsigned char* src = pixels;
            unsigned char* dst = pixelBuffer.data();
            for (int h = 0; h < height; h++) {
                if (bpp != 2) {
                    memcpy(dst, src, width * nChannels);
                    dst += width * nChannels;
                } else {
                    // rgb565, need convertion
                    for (int w = 0; w < width; w++) {
                        unsigned char r = src[w * 2 + 1] >> 3;
                        unsigned char g =
                                (src[w * 2 + 1] & 7) << 3 | src[w * 2] >> 5;
                        unsigned char b = src[w * 2] & 63;
                        r = r << 3 | r >> 2;
                        g = g << 2 | g >> 4;
                        b = b << 3 | b >> 2;
                        *(dst++) = r;
                        *(dst++) = g;
                        *(dst++) = b;
                    }
                }
                src += lineSize;
            }
            pixels = pixelBuffer.data();
        } else {
            // Just copy the pixels to our buffer.
            pixelBuffer.insert(pixelBuffer.end(), &pixels[0], &pixels[width * height * nChannels]);
        }
    }
    // We only convert png at this time..
    if (desiredFormat == ImageFormat::PNG) {
        std::vector<uint8_t> pngData;
        png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                                NULL, NULL);
        png_infop pi = png_create_info_struct(p);
        png_set_write_fn(
                p, &pngData,
                [](png_structp png_ptr, png_bytep data, png_size_t length) {
                    std::vector<uint8_t>* vec =
                            reinterpret_cast<std::vector<uint8_t>*>(
                                    png_get_io_ptr(png_ptr));
                    vec->insert(vec->end(), &data[0], &data[length]);
                },
                [](png_structp png_ptr) {});
        // already rotated through rendering
        rotation = renderer ? SKIN_ROTATION_0 : rotation;
        write_png_user_function(p, pi, nChannels, width,
                                height, rotation,
                                pixelBuffer.data());
        png_destroy_write_struct(&p, &pi);
        return Image((uint16_t)width, (uint16_t)height, nChannels, ImageFormat::PNG, pngData);
    }
    return Image((uint16_t)width, (uint16_t)height, nChannels, outputFormat, pixelBuffer);
}

bool captureScreenshot(
        emugl::Renderer* renderer,
        std::function<void(int* w,
                           int* h,
                           int* lineSize,
                           int* bytesPerPixel,
                           uint8_t** frameBufferData)> getFrameBuffer,
        SkinRotation rotation,
        android::base::StringView outputDirectoryPath,
        std::string* pOutputFilepath,
        int displayId) {
    if (!renderer && !getFrameBuffer) {
        LOG(WARNING) << "Cannot take screenshots.\n";
        return false;
    }

    Image img = takeScreenshot(ImageFormat::RAW, rotation, renderer, getFrameBuffer, displayId);

    if (img.getWidth() == 0 || img.getHeight() == 0) {
        LOG(ERROR) << "take screenshot failed";
        return false;
    }

    // If the given directory path is actually a filename with a protobuf
    // extension, write a serialized Observation instead.
    std::string outputFilePath;
    if (str_ends_with(outputDirectoryPath.data(), ".pb")) {
        std::ofstream file(outputDirectoryPath.data(), std::ios_base::binary);
        if (!file) {
            LOG(ERROR) << "Failed to open " << outputDirectoryPath;
            return false;
        }

        Observation observation;
        observation.set_timestamp_us(
                android::base::System::get()->getUnixTimeUs());
        Observation::Image* screen = observation.mutable_screen();
        screen->set_width(img.getWidth());
        screen->set_height(img.getHeight());
        screen->set_num_channels(img.getChannels());
        screen->set_data(img.getPixelBuf(), img.getPixelCount());
        observation.SerializeToOstream(&file);
        return true;
    }

    // Custom filename
    if (str_ends_with(outputDirectoryPath.data(), ".png")) {
        outputFilePath = outputDirectoryPath;
    } else {
        char fileName[100];
        // the file name is ~25 characters
        int fileNameSize = snprintf(
                fileName, sizeof(fileName), "Screenshot_%lld.png",
                (long long int)android::base::System::get()->getUnixTime());
        assert(fileNameSize < sizeof(fileName));
        (void)fileNameSize;

        outputFilePath =
                android::base::PathUtils::join(outputDirectoryPath, fileName);
    }
    if (pOutputFilepath) {
        *pOutputFilepath = outputFilePath;
    }

    // already rotated through rendering
    rotation = renderer ? SKIN_ROTATION_0 : rotation;
    savepng(outputFilePath.c_str(), img.getChannels(), img.getWidth(),
            img.getHeight(), rotation, img.getPixelBuf());
    return true;
}

}  // namespace emulation
}  // namespace android
