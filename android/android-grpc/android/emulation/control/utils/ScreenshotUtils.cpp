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

#include <array>    // for array
#include <utility>  // for pair

#include "android/base/Log.h"  // for LogStream, LOG, LogMessage

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

const std::array<RotationPair, 4> rotationMap{
        {{Rotation::PORTRAIT, SKIN_ROTATION_0},
         {Rotation::LANDSCAPE, SKIN_ROTATION_90},
         {Rotation::REVERSE_PORTRAIT, SKIN_ROTATION_180},
         {Rotation::REVERSE_LANDSCAPE, SKIN_ROTATION_270}}};

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

static constexpr bool match(Rotation_SkinRotation x, SkinRotation y) {
    return static_cast<int>(x) == static_cast<int>(y);
}

SkinRotation ScreenshotUtils::translate(Rotation_SkinRotation x) {
    // Make sure nobody accidentally renumbers the expected enums.
    static_assert(match(Rotation::PORTRAIT, SKIN_ROTATION_0));
    static_assert(match(Rotation::LANDSCAPE, SKIN_ROTATION_90));
    static_assert(match(Rotation::REVERSE_PORTRAIT, SKIN_ROTATION_180));
    static_assert(match(Rotation::REVERSE_LANDSCAPE, SKIN_ROTATION_270));
    assert(x <= static_cast<int>(SKIN_ROTATION_270));
    return static_cast<SkinRotation>(x);
};

Rotation_SkinRotation ScreenshotUtils::coarseRotation(double zaxis) {
    while (zaxis < 0.0) {
        zaxis += 360.0;
    }

    while (zaxis > 360.0) {
        zaxis -= 360.0;
    }

    if (zaxis >= 315.0 || zaxis < 45.0) {
        return Rotation::PORTRAIT;
    }

    if (zaxis >= 45.0 && zaxis < 135.0) {
        return Rotation::LANDSCAPE;
    }

    if (zaxis >= 135.0 && zaxis < 225.0) {
        return Rotation::REVERSE_PORTRAIT;
    }

    if (zaxis >= 225.0 && zaxis < 315.0) {
        return Rotation::REVERSE_LANDSCAPE;
    }

    return Rotation::PORTRAIT;
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

}  // namespace control
}  // namespace emulation
}  // namespace android
