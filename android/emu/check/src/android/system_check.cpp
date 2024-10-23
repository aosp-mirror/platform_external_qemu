// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "absl/strings/str_format.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/emulation/compatibility_check.h"

#include "host-common/FeatureControl.h"  // for isEnabled

namespace android {
namespace emulation {

using android::base::System;

// A check to make sure various system properties (OS, CPU, RAM) are
// supported for the target AVD
AvdCompatibilityCheckResult hasSufficientSystem(AvdInfo* avd) {
    if (avd == nullptr) {
        return {
                .description =
                        "No avd present, cannot check for system capabilities.",
                .status = AvdCompatibility::Warning,
        };
    }

    // Allow users and tests to skip compatibility checks
    if (System::get()->envGet("ANDROID_EMU_SKIP_SYSTEM_CHECKS") == "1") {
        return {
                .description = "System compatibility checks are disabled",
                .status = AvdCompatibility::Warning,
        };
    }

    const char* avdName = avdInfo_getName(avd);

    // Check number of cores
    const int numCores = System::get()->getCpuCoreCount();
    const int minNumCores = 2;
    const int idealMinNumCores = 4;
    if (numCores < minNumCores) {
        return {
                .description =
                        absl::StrFormat("Not enough number of CPU cores to run "
                                        "avd: '%s'. Available: %d, minimum "
                                        "required: %d",
                                        avdName, numCores, minNumCores),
                .status = AvdCompatibility::Error,
        };
    } else if (numCores < idealMinNumCores) {
        return {
                .description = absl::StrFormat(
                        "Suggested minimum number of CPU cores to run "
                        "avd '%s' is %d (available: %d).",
                        avdName, idealMinNumCores, numCores),
                .status = AvdCompatibility::Warning,
        };
    }

    // Check system RAM
    const android::base::MemUsage memUsage = System::get()->getMemUsage();
    if (memUsage.total_phys_memory == 0) {
        return {
                .description = absl::StrFormat(
                        "Could not check available system memory"),
                .status = AvdCompatibility::Warning,
        };
    }
    const uint64_t ramMB = (memUsage.total_phys_memory / (1024 * 1024));
    const uint64_t minRamMB = 2048;
    const uint64_t idealMinRamMB = 8192;
    if (ramMB < minRamMB) {
        return {
                .description = absl::StrFormat(
                        "Available system RAM is not enough to run "
                        "avd: '%s'. Available: %d, minimum "
                        "required: %d",
                        avdName, ramMB, minRamMB),
                .status = AvdCompatibility::Error,
        };
    } else if (ramMB < idealMinRamMB) {
        return {
                .description =
                        absl::StrFormat("Suggested minimum system RAM to run "
                                        "avd '%s' is %d MB (available: %d MB).",
                                        avdName, idealMinRamMB, ramMB),
                .status = AvdCompatibility::Warning,
        };
    }

    return {
            .description = absl::StrFormat(
                    "System requirements to run avd: `%s` are met.", avdName),
            .status = AvdCompatibility::Ok,
    };
}

REGISTER_COMPATIBILITY_CHECK(hasSufficientSystem);

}  // namespace emulation
}  // namespace android
