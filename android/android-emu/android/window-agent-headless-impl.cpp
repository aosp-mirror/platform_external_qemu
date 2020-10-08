// Copyright 2015-2017 The Android Open Source Project
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

#include <math.h>    // for fabs
#include <stdint.h>  // for uint32_t, int32_t

#include <array>    // for array
#include <utility>  // for make_pair, pair

#include "android/emulation/control/sensors_agent.h"   // for QAndroidSensor...
#include "android/emulation/control/window_agent.h"    // for WINDOW_MESSAGE...
#include "android/emulator-window.h"                   // for emulator_windo...
#include "android/hw-sensors.h"                        // for ANDROID_SENSOR...
#include "android/skin/qt/emulator-no-qt-no-window.h"  // for EmulatorNoQtNo...
#include "android/skin/rect.h"                         // for (anonymous)
#include "android/utils/debug.h"                       // for derror, dprint
#include "glm/detail/func_geometric.hpp"               // for dot, normalize
#include "glm/detail/type_vec.hpp"                     // for vec3
#include "glm/detail/type_vec3.hpp"                    // for tvec3

static SkinRotation getRotation() {
    // Calculate the skin rotation based upon the state of the physical sensors.
    EmulatorWindow* window = emulator_window_get();
    if (!window)
        return SKIN_ROTATION_0;
    const QAndroidSensorsAgent* sensorsAgent = window->uiEmuAgent->sensors;
    if (!sensorsAgent)
        return SKIN_ROTATION_0;
    sensorsAgent->advanceTime();
    glm::vec3 device_accelerometer;
    sensorsAgent->getSensor(ANDROID_SENSOR_ACCELERATION,
                            &device_accelerometer.x, &device_accelerometer.y,
                            &device_accelerometer.z);
    glm::vec3 normalized_accelerometer = glm::normalize(device_accelerometer);

    static const std::array<std::pair<glm::vec3, SkinRotation>, 4> directions{
            std::make_pair(glm::vec3(0.0f, 1.0f, 0.0f), SKIN_ROTATION_0),
            std::make_pair(glm::vec3(-1.0f, 0.0f, 0.0f), SKIN_ROTATION_90),
            std::make_pair(glm::vec3(0.0f, -1.0f, 0.0f), SKIN_ROTATION_180),
            std::make_pair(glm::vec3(1.0f, 0.0f, 0.0f), SKIN_ROTATION_270)};
    SkinRotation coarse_orientation = SKIN_ROTATION_0;
    for (const auto& v : directions) {
        if (fabs(glm::dot(normalized_accelerometer, v.first) - 1.f) < 0.1f) {
            coarse_orientation = v.second;
            break;
        }
    }
    return coarse_orientation;
}

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = emulator_window_get,
        .rotate90Clockwise = [] { return emulator_window_rotate_90(true); },
        .rotate = emulator_window_rotate,
        .getRotation = [] { return getRotation(); },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                },
        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                    // Don't necessarily perform the func since the
                    // user doesn't get a chance to dismiss.
                },
        .fold =
                [](bool is_fold) -> bool {
                    if (is_fold) {
                        return android_foldable_fold();
                    } else {
                        return android_foldable_unfold();
                    }
                },
        .isFolded =
                []() -> bool {
                    return android_foldable_is_folded();
                },
        .getFoldedArea =
                [](int* x, int* y, int* w, int* h) -> bool {
                    return android_foldable_get_folded_area(x, y, w, h);
                },
        .updateFoldablePostureIndicator = [](bool) {},
        .setUIDisplayRegion = [](int x, int y, int w, int h) {},
        .getMultiDisplay = [](uint32_t id,
                              int32_t* x,
                              int32_t* y,
                              uint32_t* w,
                              uint32_t* h,
                              uint32_t* dpi,
                              uint32_t* flag,
                              bool* enabled) -> bool { return false; },
        .setNoSkin = []() {},
        .restoreSkin = []() {},
        .updateUIMultiDisplayPage = [](uint32_t id) {},
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) -> bool {
                    if (w)
                        *w = 2500;
                    if (h)
                        *h = 1600;
                    return true;
                },
        .startExtendedWindow =
                []() {
                // Not implemented
                return false;
                },
        .quitExtendedWindow =
                []() {
                // Not implemented
                return false;
                },
        .setUiTheme = [](SettingsTheme type) {
                // Not implemented
                return false;
                },
};

extern "C" const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
