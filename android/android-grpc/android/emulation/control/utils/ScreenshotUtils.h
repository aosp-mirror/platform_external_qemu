// Copyright (C) 2019 The Android Open Source Project
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

#include <tuple>  // for tuple

#include "android/emulation/control/ScreenCapturer.h"  // for ImageFormat
#include "android/skin/rect.h"                         // for SkinRotation
#include "emulator_controller.pb.h"                    // for ImageFormat_Im...

namespace android {
namespace emulation {
namespace control {

// Contains a set of convenience methods to assist with creating screenshots.
class ScreenshotUtils {
public:
    // Translates our external format to the intenal format. Note that the
    // internal format contains values that do not exist in the external enum.
    static android::emulation::ImageFormat translate(ImageFormat_ImgFormat x);

    // Translates our internal format to the external format. Note that the
    // internal format contains values that do not exist in the external enum.
    static ImageFormat_ImgFormat translate(android::emulation::ImageFormat x);

    // Translates from our external protobuf format to the internal qemu
    // representation.
    static SkinRotation translate(Rotation_SkinRotation x);

    // Translates the zaxis rotation ([-180, 180]) to the coarse grained android
    // rotation.
    static Rotation_SkinRotation coarseRotation(double zaxis);

    // Calculates a resize from (w,h) -> (desiredWidth, desiredHeight) while
    // maintaining the aspect ratio. Returns a tuple with the resulting (w,h)
    // that should be used.
    static std::tuple<int, int> resizeKeepAspectRatio(double width,
                                                      double height,
                                                      double desiredWidth,
                                                      double desiredHeight);
};

}  // namespace control
}  // namespace emulation
}  // namespace android
