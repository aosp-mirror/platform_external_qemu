// Copyright 2022 The Android Open Source Project
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
#include "android/crashreport/ui/UserSuggestions.h"

#include <algorithm>                                  // for transform
#include <cctype>                                     // for tolower
#include <string>                                     // for string
#include <vector>                                     // for vector
#include "absl/strings/match.h"
#include "google_breakpad/processor/call_stack.h"     // for CallStack
#include "google_breakpad/processor/code_module.h"    // for CodeModule
#include "google_breakpad/processor/process_state.h"  // for ProcessState
#include "google_breakpad/processor/stack_frame.h"    // for StackFrame
#include "processor/pathname_stripper.h"              // for PathnameStripper

namespace android {
namespace crashreport {

// This is a list of all substrings that definitely belong to some graphics
// drivers.
// NB: all names have to be lower-case - we transform the string from the stack
// trace to lowe case before doing the comparisons

static const char* const gfx_drivers_lcase[] = {
        // NVIDIA
        "nvcpl",            // Control Panel
        "nvshell",          // Desktop Explorer
        "nvinit",           // initializer
        "nv4_disp",         // Windows 2000 (!) display driver
        "nvcod",            // CoInstaller
        "nvcuda",           // Cuda
        "nvopencl",         // OpenCL
        "nvcuvid",          // Video decode
        "nvogl",            // OpenGL (modern)
        "ig4icd",           // OpenGL (icd?)
        "geforcegldriver",  // OpenGL (old)
        "nvd3d",            // Direct3D
        "nvwgf2",           // D3D10
        "nvdx",             // D3D shim drivers
        "nvml",             // management library
        "nvfbc",            // front buffer capture library
        "nvapi",            // NVAPI Library
        "libnvidia",        // NVIDIA Linux

        // ATI
        "atioglxx",  // OpenGL
        "atig6txx",  // OpenGL
        "r600",      // Radeon r600 series
        "aticfx",    // DX11
        "atiumd",    // D3D
        "atidxx",    // "TMM Com Clone Control Module" (???)
        "atimpenc",  // video encoder
        "atigl",     // another ATI OpenGL driver
        "amdvlk",    // AMD Vulkan

        // Intel
        "i915",      // Intel i915 gpu
        "igd",       // 'igd' series of Intel GPU's
        "igl",       // ?
        "igfx",      // ?
        "ig75icd",   // Intel icd
        "intelocl",  // Intel OpenCL

        // Others
        "libgl.",        // Low-level Linux OpenGL
        "opengl32.dll",  // Low-level Windows OpenGL
};

// List of gfx driver hooks that are known to crash
static const char* const gfx_hooks_lcase[] = {
        "bdcamvk",   // Bandicam Vulkan hook
        "fpsmonvk",  // FPS monitor hook
};

// Suggestions for each gfx driver hook, by index in |gfx_hooks_lcase|
static const char* const gfx_hooks_suggestions[] = {
        // Bandicam
        "It appears Bandicam Vulkan hooks are installed on your system, "
        "which can be causing the crash. Try uninstalling Bandicam / removing "
        "the hooks.",
        // FPS monitor
        "It appears FPS Monitor Vulkan hooks are installed, "
        "which can be causing the crash. "
        "Try running without the monitor or uninstalling FPS Monitor. ",
};

static_assert(sizeof(gfx_hooks_lcase) == sizeof(gfx_hooks_suggestions),
              "Each crashy graphics hook must come with a suggestion");

static bool containsGfxPattern(const std::string& str) {
    for (const char* pattern : gfx_drivers_lcase) {
        if (absl::StrContains(str, pattern)) {
            return true;
        }
    }

    return false;
}

static bool containsGfxHookPattern(const std::string& str) {
    for (const char* pattern : gfx_hooks_lcase) {
        if (absl::StrContains(str, pattern)) {
            return true;
        }
    }
    return false;
}

static int getGfxHookIndex(const std::string& str) {
    const int defaultRes = -1;
    int i = 0;
    for (const char* pattern : gfx_hooks_lcase) {
        if (absl::StrContains(str, pattern)) {
            return i;
        }
        ++i;
    }
    return defaultRes;
}

UserSuggestions::UserSuggestions(google_breakpad::ProcessState* process_state) {
    suggestions.clear();

    // Go through all frames of the stack of
    // the thread requesting dump.

    int crashed_thread_id = process_state->requesting_thread();
    if (crashed_thread_id < 0 ||
        crashed_thread_id >= static_cast<int>(process_state->threads()->size())) {
        return;
    }
    google_breakpad::CallStack* crashed_stack =
            process_state->threads()->at(crashed_thread_id);
    if (!crashed_stack) {
        return;
    }

    const int frame_count = crashed_stack->frames()->size();
    for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
        google_breakpad::StackFrame* const frame =
                crashed_stack->frames()->at(frame_index);
        if (!frame) {
            continue;
        }
        const auto& module = frame->module;

        if (module) {
            std::string file = google_breakpad::PathnameStripper::File(
                    module->code_file());
            std::transform(file.begin(), file.end(), file.begin(), ::tolower);

            // Should the user update their graphics drivers?
            if (containsGfxPattern(file)) {
                suggestions.insert(Suggestion::UpdateGfxDrivers);
            }

            // Should the user disable a graphics hook?
            if (containsGfxHookPattern(file)) {
                int gfxHookIndex = getGfxHookIndex(file);
                if (gfxHookIndex != -1) {
                    stringSuggestions.insert(
                            gfx_hooks_suggestions[gfxHookIndex]);
                }
                break;
            }
        }
    }
}
}  // namespace crashreport
}  // namespace android
