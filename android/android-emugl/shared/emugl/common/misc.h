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

#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/window_agent.h"
#include "android/opengl/emugl_config.h"

#ifdef _MSC_VER
# ifdef BUILDING_EMUGL_COMMON_SHARED
#  define EMUGL_COMMON_API __declspec(dllexport)
# else
#  define EMUGL_COMMON_API __declspec(dllimport)
#endif
#else
# define EMUGL_COMMON_API
#endif

namespace android {

namespace base {

class CpuUsage;
class MemoryTracker;
class GLObjectCounter;

} // namespace base
} // namespace android

namespace emugl {

    // Set and get API version of system image.
    EMUGL_COMMON_API void setAvdInfo(bool isPhone, int apiLevel);
    EMUGL_COMMON_API void getAvdInfo(bool* isPhone, int* apiLevel);

    // Set/get GLES major/minor version.
    EMUGL_COMMON_API void setGlesVersion(int maj, int min);
    EMUGL_COMMON_API void getGlesVersion(int* maj, int* min);

    // Set/get renderer
    EMUGL_COMMON_API void setRenderer(SelectedRenderer renderer);
    EMUGL_COMMON_API SelectedRenderer getRenderer();

    // Extension string query
    EMUGL_COMMON_API bool hasExtension(const char* extensionsStr,
                      const char* wantedExtension);

    // GL object counter get/set
    EMUGL_COMMON_API void setGLObjectCounter(
            android::base::GLObjectCounter* counter);
    EMUGL_COMMON_API android::base::GLObjectCounter* getGLObjectCounter();

    // CPU usage get/set.
    EMUGL_COMMON_API void setCpuUsage(android::base::CpuUsage* usage);
    EMUGL_COMMON_API android::base::CpuUsage* getCpuUsage();

    // Memory usage get/set
    EMUGL_COMMON_API void setMemoryTracker(android::base::MemoryTracker* usage);
    EMUGL_COMMON_API android::base::MemoryTracker* getMemoryTracker();

    // Window operation agent
    EMUGL_COMMON_API void set_emugl_window_operations(const QAndroidEmulatorWindowAgent &voperations);
    EMUGL_COMMON_API const QAndroidEmulatorWindowAgent &get_emugl_window_operations();

    // MultiDisplay operation agent
    EMUGL_COMMON_API void set_emugl_multi_display_operations(const QAndroidMultiDisplayAgent &operations);
    EMUGL_COMMON_API const QAndroidMultiDisplayAgent &get_emugl_multi_display_operations();

}
