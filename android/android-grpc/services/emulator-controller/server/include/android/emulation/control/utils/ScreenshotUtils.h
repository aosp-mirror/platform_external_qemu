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
#include "android/emulation/control/sensors_agent.h"   // for QAndroidSensor...
#include "android/skin/rect.h"                         // for SkinRotation
#include "emulator_controller.pb.h"                    // for Rotation_SkinR...

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

    // Derives the current device rotation from the sensor state
    static Rotation_SkinRotation deriveRotation(
            const QAndroidSensorsAgent* sensorAgent);

    // Calculates a resize from (w,h) -> (desiredWidth, desiredHeight) while
    // maintaining the aspect ratio. Returns a tuple with the resulting (w,h)
    // that should be used.
    static std::tuple<int, int> resizeKeepAspectRatio(double width,
                                                      double height,
                                                      double desiredWidth,
                                                      double desiredHeight);

    static bool equals(const DisplayConfiguration& a,
                       const DisplayConfiguration& b);

    // Gets the bytes per pixel for the given format.
    static int getBytesPerPixel(const ImageFormat& fmt);

    // Returns true if we have the host side open gles renderer
    static bool hasOpenGles();

    // Takes a screenshot from the given display id
    // in the desired format, rotation, width, height and
    // place it in the pre allocated pixel buffer, with the final computed width
    // and height.
    //
    // If this function returns false and *cPixels != 0 then the pixel buffer
    // needs to be resized to at least cPixels bytes.
    //
    // The usual use of this is to use the first call to figure out how much
    // memory needs to be allocated, and the subsequent call to actually fetch the
    // image once the memory is allocated.
    //
    // Note: This function can be very expensive under the following circumstances:
    //   - You are requesting the PNG format.
    //   - You are running on an older api level without a host renderer

    // This function supports rectangle snipping by
    // providing an |rect| parameter. The default value of {{0,0}, {0,0}}
    // indicates the users wants to snip the entire screen instead of a
    // partial screen.
    // - |rect|  represents a rectangle within the screen defined by
    // desiredWidth and desiredHeight.
    static bool getScreenshot(int displayId,
                              const ImageFormat_ImgFormat format,
                              const Rotation_SkinRotation rotation,
                              const uint32_t desiredWidth,
                              const uint32_t desiredHeight,
                              uint8_t* pixels,
                              size_t* cPixels,
                              uint32_t* finalWidth,
                              uint32_t* finalHeight,
                              SkinRect rect = {{0, 0}, {0, 0}});

    // Takes a screenshot from the given display id
    // in the desired format, rotation, width, height and
    // return the captured image with the final computed width
    // and height.
    static android::emulation::Image getScreenshot(
            int displayId,
            const ImageFormat_ImgFormat format,
            const Rotation_SkinRotation rotation,
            const uint32_t desiredWidth,
            const uint32_t desiredHeight,
            uint32_t* finalWidth,
            uint32_t* finalHeight,
            SkinRect rect = {{0, 0}, {0, 0}});
};

}  // namespace control
}  // namespace emulation
}  // namespace android
