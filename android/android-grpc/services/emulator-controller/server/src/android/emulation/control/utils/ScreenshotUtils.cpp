// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/utils/ScreenshotUtils.h"

#include <assert.h>  // for assert
#include <math.h>    // for fabs
#include <array>     // for array
#include <ostream>   // for operator<<, bas...
#include <utility>   // for make_pair, pair

#include "aemu/base/Log.h"  // for LogStreamVoidify
#include "aemu/base/Tracing.h"
#include "android/console.h"
#include "android/emulation/control/sensors_agent.h"  // for QAndroidSensors...
#include "android/hw-sensors.h"                       // for ANDROID_SENSOR_...
#include "host-common/opengles.h"
#include "android/physics/GlmHelpers.h"  // for vecNearEqual

#ifdef _WIN32
#undef ERROR
#endif

namespace android {
namespace emulation {
namespace control {

using ImageFormatPair =
        std::pair<ImageFormat_ImgFormat, android::emulation::ImageFormat>;
using RotationPair = std::pair<Rotation_SkinRotation, SkinRotation>;

const std::array<ImageFormatPair, 3> imgFormatMap{
        {{ImageFormat::RGBA8888, android::emulation::ImageFormat::RGBA8888},
         {ImageFormat::PNG, android::emulation::ImageFormat::PNG},
         {ImageFormat::RGB888, android::emulation::ImageFormat::RGB888}}};

// Translates our external format to the intenal format. Note that the
// internal format contains values that do not exist in the external enum.
android::emulation::ImageFormat ScreenshotUtils::translate(
        ImageFormat_ImgFormat x) {
    for (auto [fst, snd] : imgFormatMap) {
        if (fst == x)
            return snd;
    }
    // Shouldn't happen.
    LOG(ERROR) << "Unknown enum: " << (int)x << ", returning RGBA8888 format.";
    return android::emulation::ImageFormat::RGBA8888;
};

// Translates our internal format to the external format. Note that the
// internal format contains values that do not exist in the external enum.
ImageFormat_ImgFormat ScreenshotUtils::translate(
        android::emulation::ImageFormat x) {
    for (auto [fst, snd] : imgFormatMap) {
        if (snd == x)
            return fst;
    }
    // Shouldn't happen.
    LOG(ERROR) << "Unknown enum: " << (int)x << ", returning RGBA8888 format.";
    return ImageFormat::RGBA8888;
};

SkinRotation ScreenshotUtils::translate(Rotation_SkinRotation x) {
    switch (x) {
        default:
        case Rotation::PORTRAIT:
            return SKIN_ROTATION_0;
        case Rotation::LANDSCAPE:
            return SKIN_ROTATION_270;
        case Rotation::REVERSE_PORTRAIT:
            return SKIN_ROTATION_180;
        case Rotation::REVERSE_LANDSCAPE:
            return SKIN_ROTATION_90;
    }
};

Rotation_SkinRotation ScreenshotUtils::deriveRotation(
        const QAndroidSensorsAgent* sensorAgent) {
    glm::vec3 device_accelerometer;
    auto out = {&device_accelerometer.x, &device_accelerometer.y,
                &device_accelerometer.z};
    sensorAgent->getSensor(ANDROID_SENSOR_ACCELERATION, out.begin(),
                           out.size());

    glm::vec3 normalized_accelerometer = glm::normalize(device_accelerometer);
    const float cos45 = sqrtf(2.0f) / 2.0f;
    if (normalized_accelerometer.y > cos45) {
        return Rotation::PORTRAIT;
    }
    if (normalized_accelerometer.y < -cos45) {
        return Rotation::REVERSE_PORTRAIT;
    }
    if (normalized_accelerometer.x > 0) {
        return Rotation::LANDSCAPE;
    }
    return Rotation::REVERSE_LANDSCAPE;
}

std::tuple<int, int> ScreenshotUtils::resizeKeepAspectRatio(
        double width,
        double height,
        double desiredWidth,
        double desiredHeight) {
    double aspectRatio = width / height;
    double newAspectRatio = desiredWidth / desiredHeight;
    int newWidth, newHeight;
    if (newAspectRatio > aspectRatio) {
        // Wider than necessary; use the same height and compute the width
        // from the desired aspect ratio.
        newHeight = desiredHeight;
        newWidth = (desiredHeight * aspectRatio);
    } else {
        // Taller than necessary; use the same width and compute the height
        // from the desired aspect ratio
        newWidth = desiredWidth;
        newHeight = (desiredWidth / aspectRatio);
    }
    return std::make_tuple(newWidth, newHeight);
}

bool ScreenshotUtils::equals(const DisplayConfiguration& a,
                             const DisplayConfiguration& b) {
    return a.width() == b.width() && a.height() == b.height() &&
           a.dpi() == b.dpi() && a.flags() == b.flags() &&
           a.display() == b.display();
}

int ScreenshotUtils::getBytesPerPixel(const ImageFormat& fmt) {
    return fmt.format() == ImageFormat::RGB888 ? 3 : 4;
}

bool ScreenshotUtils::hasOpenGles() {
    return android_getOpenglesRenderer().get() != nullptr;
}

bool ScreenshotUtils::getScreenshot(int displayId,
                                    const ImageFormat_ImgFormat format,
                                    const Rotation_SkinRotation rotation,
                                    const uint32_t desiredWidth,
                                    const uint32_t desiredHeight,
                                    uint8_t* pixels,
                                    size_t* cPixels,
                                    uint32_t* finalWidth,
                                    uint32_t* finalHeight,
                                    SkinRect rect) {
    AEMU_SCOPED_TRACE("ScreenshotUtils::getScreenshot");
    // Screenshots can come from either the gl renderer, or the guest.
    const auto& renderer = android_getOpenglesRenderer();
    android::emulation::ImageFormat desiredFormat =
            ScreenshotUtils::translate(format);
    SkinRotation desiredRotation = ScreenshotUtils::translate(rotation);
    if (renderer.get() &&
        (format == ImageFormat::RGB888 || format == ImageFormat::RGBA8888)) {
        unsigned int bpp = (format == ImageFormat::RGB888 ? 3 : 4);
        return renderer.get()->getScreenshot(
                       bpp, finalWidth, finalHeight, pixels, cPixels, displayId,
                       desiredWidth, desiredHeight, desiredRotation,
                       {{rect.pos.x, rect.pos.y}, {rect.size.w, rect.size.h}}) == 0;
    } else {
        // oh, oh slow path.
        android::emulation::Image img = android::emulation::takeScreenshot(
                desiredFormat, desiredRotation, renderer.get(),
                getConsoleAgents()->display->getFrameBuffer, displayId,
                desiredWidth, desiredHeight, rect);
        if (img.getPixelCount() > *cPixels) {
            *cPixels = img.getPixelCount();
            return false;
        }
        *cPixels = img.getPixelCount();
        memcpy(pixels, img.getPixelBuf(), *cPixels);
        *finalWidth = img.getWidth();
        *finalHeight = img.getHeight();
    }

    return true;
}

android::emulation::Image ScreenshotUtils::getScreenshot(
        int displayId,
        const ImageFormat_ImgFormat format,
        const Rotation_SkinRotation rotation,
        const uint32_t desiredWidth,
        const uint32_t desiredHeight,
        uint32_t* finalWidth,
        uint32_t* finalHeight,
        SkinRect rect) {
    const auto& renderer = android_getOpenglesRenderer();
    android::emulation::ImageFormat desiredFormat =
            ScreenshotUtils::translate(format);
    SkinRotation desiredRotation = ScreenshotUtils::translate(rotation);
    auto img = android::emulation::takeScreenshot(
            desiredFormat, desiredRotation, renderer.get(),
            getConsoleAgents()->display->getFrameBuffer, displayId,
            desiredWidth, desiredHeight, rect);
    *finalWidth = img.getWidth();
    *finalHeight = img.getHeight();
    return img;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
