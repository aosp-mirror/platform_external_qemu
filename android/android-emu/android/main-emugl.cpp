// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/main-emugl.h"

#include "android/avd/util.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/opengl/gpuinfo.h"
#include "android/utils/debug.h"
#include "android/utils/string.h"

#include <stdlib.h>
#include <string.h>

using android::base::ScopedCPtr;

bool androidEmuglConfigInit(EmuglConfig* config,
                            const char* avdName,
                            const char* avdArch,
                            int apiLevel,
                            bool hasGoogleApis,
                            const char* gpuOption,
                            char** hwGpuModePtr,
                            int wantedBitness,
                            bool noWindow,
                            enum WinsysPreferredGlesBackend uiPreferredBackend,
                            bool* hostGpuVulkanBlacklisted,
                            bool forceUseHostGpuVulkan) {
    bool gpuEnabled = false;

    bool avdManagerHasWrongSetting = apiLevel < 16;

    if (avdManagerHasWrongSetting &&
        (*hwGpuModePtr) &&
        !gpuOption &&
        (!strcmp(*hwGpuModePtr, "off") ||
         !strcmp(*hwGpuModePtr, "guest"))) {
        str_reset(hwGpuModePtr, "auto");
    }

    if (avdName) {
        gpuEnabled = hwGpuModePtr && (*hwGpuModePtr);
        // If the user has hw config set to mesa, not-so-silently overrule that.
        if (!gpuOption && !strcmp(*hwGpuModePtr, "mesa")) {
            dwarning(
                    "Your AVD has been configured with the Mesa renderer, "
                    "which is deprecated. This AVD is being auto-switched to "
                    "the current and better-supported \'swiftshader\' renderer. "
                    "Please update your AVD config.ini's hw.gpu.mode to match.");
            str_reset(hwGpuModePtr, "swiftshader_indirect");
        }
    } else if (!gpuOption) {
        // In the case of a platform build, use the 'auto' mode by default.
        str_reset(hwGpuModePtr, "auto");
        gpuEnabled = true;
    }

    // Detect if this is google API's

    bool hasGuestRenderer = (!strcmp(avdArch, "x86") ||
                             !strcmp(avdArch, "x86_64")) &&
                             (apiLevel >= 23) &&
                             hasGoogleApis;

    bool blacklisted = false;
    bool onBlacklist = false;

    const char* gpuChoice = gpuOption ? gpuOption : *hwGpuModePtr;
    // If the user has specified a renderer
    // that is neither "auto", "host" nor "on",
    // don't check the blacklist.
    // Only check the blacklist for 'auto', 'host' or 'on' mode.
    if (gpuChoice && (!strcmp(gpuChoice, "auto") ||
                      !strcmp(gpuChoice, "host") ||
                      !strcmp(gpuChoice, "auto-no-window") ||
                      !strcmp(gpuChoice, "on"))) {

         onBlacklist = isHostGpuBlacklisted();
    }

    if (gpuChoice && (!strcmp(gpuChoice, "auto") || !strcmp(gpuChoice, "auto-no-window"))) {
        if (onBlacklist) {
            dwarning("Your GPU drivers may have a bug. "
                     "Switching to software rendering.");
        }
        blacklisted = onBlacklist;
        setGpuBlacklistStatus(blacklisted);
    } else if (onBlacklist && gpuChoice &&
            (!strcmp(gpuChoice, "host") || !strcmp(gpuChoice, "on"))) {
        dwarning("Your GPU drivers may have a bug. "
                 "If you experience graphical issues, "
                 "please consider switching to software rendering.");
    }

    bool result = emuglConfig_init(
            config, gpuEnabled, *hwGpuModePtr, gpuOption, wantedBitness,
            noWindow, blacklisted, hasGuestRenderer, uiPreferredBackend,
            forceUseHostGpuVulkan);

    *hostGpuVulkanBlacklisted =
        async_query_host_gpu_VulkanBlacklisted();

    return result;
}
