/*
* Copyright (C) 2022 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "android/avd/info.h"

namespace android {
namespace automotive {
struct DisplayManager {
    // Constants copied from DisplayManager. For all flags see:
    // frameworks/base/core/java/android/hardware/display/DisplayManager.java
    enum {
        VIRTUAL_DISPLAY_FLAG_PUBLIC = 1 << 0,
        VIRTUAL_DISPLAY_FLAG_PRESENTATION = 1 << 1,
        VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY = 1 << 3,
        VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH = 1 << 6,
        VIRTUAL_DISPLAY_FLAG_ROTATES_WITH_CONTENT = 1 << 7,
        VIRTUAL_DISPLAY_FLAG_DESTROY_CONTENT_ON_REMOVAL = 1 << 8,
        VIRTUAL_DISPLAY_FLAG_SHOULD_SHOW_SYSTEM_DECORATIONS = 1 << 9,
        VIRTUAL_DISPLAY_FLAG_TRUSTED = 1 << 10,
    };
};
const int DEFAULT_FLAGS_AUTO =
        DisplayManager::VIRTUAL_DISPLAY_FLAG_PUBLIC |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_ROTATES_WITH_CONTENT |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_TRUSTED |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_SHOULD_SHOW_SYSTEM_DECORATIONS |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_DESTROY_CONTENT_ON_REMOVAL;

const int DEFAULT_FLAGS_AUTO_CLUSTER =
        DisplayManager::VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_TRUSTED |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH |
        DisplayManager::VIRTUAL_DISPLAY_FLAG_DESTROY_CONTENT_ON_REMOVAL;

// The build property key for multi display support
static const char kMultiDisplayProp[] = "ro.emulator.car.multidisplay";

// The build property key for distant display support
static const char kDistantDisplayProp[] = "ro.emulator.car.distantdisplay";

// Returns the DisplayManager flags to use for the given display ID.
int getDefaultFlagsForDisplay(int displayId);
// Returns true when multidisplay is supported by the automotive system.
bool isMultiDisplaySupported(const AvdInfo* i);
// Returns true when distantdisplay is supported by the automotive system.
bool isDistantDisplaySupported(const AvdInfo* info);
} // namespace automotive
} // namespace android
