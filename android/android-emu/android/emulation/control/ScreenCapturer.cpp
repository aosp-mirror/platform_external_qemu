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

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/emulation/control/display_agent.h"
#include "android/emulator-window.h"
#include "android/loadpng.h"
#include "android/opengles.h"

namespace android {
namespace emulation {

bool captureScreenshot(android::base::StringView outputDirectoryPath,
                       std::string* pOutputFilepath) {
    const auto& renderer = android_getOpenglesRenderer();
    if (const auto renderer_ptr = renderer.get()) {
        return captureScreenshot(renderer_ptr, nullptr, outputDirectoryPath,
                                 pOutputFilepath);
    } else {
        // renderer is nullptr in -gpu guest
        return captureScreenshot(nullptr,
                emulator_window_get()->uiEmuAgent->display->getFrameBuffer,
                outputDirectoryPath, pOutputFilepath);
    }
}

bool captureScreenshot(emugl::Renderer* renderer,
                       std::function<void(int* w, int* h, int* lineSize,
                            int* bytesPerPixel, uint8_t** frameBufferData)>
                            getFrameBuffer,
                       android::base::StringView outputDirectoryPath,
                       std::string* pOutputFilepath) {
    if (!renderer && !getFrameBuffer) {
        LOG(WARNING) << "Cannot take screenshots.\n";
        return false;
    }
    unsigned int nChannels = 4;
    unsigned int width;
    unsigned int height;
    std::vector<unsigned char> pixelBuffer;
    unsigned char* pixels = nullptr;
    if (renderer) {
        renderer->getScreenshot(nChannels, &width, &height, pixelBuffer);
        pixels = pixelBuffer.data();
    } else {
        int bpp = 4;
        int lineSize = 0;
        getFrameBuffer((int*)&width, (int*)&height, &lineSize, &bpp,
                &pixels);
        if (bpp < 1 || bpp > 4) {
            // unknown pixel buffer format
            return false;
        }
        // -gpu guest usually gives us bpp=2
        // bpp=2 infers we are using rgb565
        // convert it to rgb888
        nChannels = bpp == 2 ? 3 : bpp;
        // Need to handle padding if lineSize != width * nChannels
        if ((lineSize != 0 && lineSize != width * nChannels) ||
                bpp == 2) {
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
                        unsigned char r = src[w*2+1] >> 3;
                        unsigned char g = (src[w*2+1] & 7) << 3 | src[w*2] >> 5;
                        unsigned char b = src[w*2] & 63;
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
        }
    }
    if (width == 0 || height == 0) {
        return false;
    }
    // the file name is ~25 characters
    char fileName[100];
    int fileNameSize = snprintf(fileName, sizeof(fileName), "Screenshot_%lld.png",
            (long long int)android::base::System::get()->getUnixTime());
    assert(fileNameSize < sizeof(fileName));
    (void)fileNameSize;

    std::string outputFilePath =
            android::base::PathUtils::join(outputDirectoryPath, fileName);
    if (pOutputFilepath) {
        *pOutputFilepath = outputFilePath;
    }
    savepng(outputFilePath.c_str(), nChannels, width, height, pixels);
    return true;
}

}  // namespace emulation
}  // namespace android
