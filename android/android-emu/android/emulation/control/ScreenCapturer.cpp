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
#include "android/loadpng.h"
#include "android/opengles.h"

namespace android {
namespace emulation {

bool captureScreenshot(android::base::StringView outputDirectoryPath) {
    const auto& renderer = android_getOpenglesRenderer();
    return captureScreenshot(renderer.get(), outputDirectoryPath);
}

bool captureScreenshot(emugl::Renderer* renderer,
        android::base::StringView outputDirectoryPath) {
    if (!renderer) {
        LOG(WARNING) << "Renderer uninitialized, cannot take screenshots.\n";
        return false;
    }
    const unsigned int nChannels = 4;
    unsigned int width;
    unsigned int height;
    std::vector<unsigned char> pixels;
    renderer->getScreenshot(nChannels, &width, &height, pixels);
    if (width == 0 || height == 0) {
        return false;
    }
    // the file name is ~25 characters
    char fileName[100];
    int fileNameSize = snprintf(fileName, sizeof(fileName), "Screenshot_%lld.png",
            (long long int)android::base::System::get()->getUnixTime());
    assert(fileNameSize < sizeof(fileName));
    (void)fileNameSize;
    savepng(android::base::PathUtils::join(outputDirectoryPath, fileName)
            .c_str(), nChannels, width, height, pixels.data());
    return true;
}

}  // namespace emulation
}  // namespace android
