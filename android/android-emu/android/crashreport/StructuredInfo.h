// Copyright 2017 The Android Open Source Project
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

#include "android/avd/info.h"
#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/structured_info.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/HostHwInfo.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"

#include <fstream>
#include <string>

namespace android {
namespace crashreport {

// StructuredInfo sets up a protobuf containing all
// pertinent information related to the emulator. The
// intended use is for crash reports to have queryable
// fields.
class StructuredInfo {
public:
    StructuredInfo() = default;

    // Global getter
    static StructuredInfo* get();

    // Functions for resetting the protobuf
    // and for (de)serialization.
    void clear();
    void toBinaryString(std::string* res) const;
    void toOstream(std::ofstream* out) const;
    void reloadFromBinaryString(const std::string& buf);
    void reloadFromFile(const std::string& filename);

    // Entry points for other code to call in order to
    // add to the protobuf.
    // Convenience initializations for
    // common emulator info.
    // initEmulatorCommonInfo():
    // sets host hw info, cpu accel, feature flags,
    // and opengl.
    void initEmulatorCommonInfo();
    // AVD info and session phase need to be set
    // separately as they occur during different
    // times.
    void setAvdInfo(AvdInfo* info);
    void setSessionPhase(AndroidSessionPhase phase);

    // Break down into smaller pieces below.
    // Some are already done in initEmulatorCommonInfo(),
    // but these are good for testing generally.
    void setHostHwInfo(const HostHwInfo::Info& info);
    void setHypervisor(CpuAccelerator accel);
    void setFeatureFlagUsage(
            const std::vector<featurecontrol::Feature>& enabledNonOverride,
            const std::vector<featurecontrol::Feature>& enabledOverride,
            const std::vector<featurecontrol::Feature>& disabledOverride,
            const std::vector<featurecontrol::Feature>& enabled);
    void setRenderer(SelectedRenderer renderer);

    void setGuestGlVendor(const char* vendor);
    void setGuestGlRenderer(const char* renderer);
    void setGuestGlVersion(const char* version);

    void setUptime(uint64_t uptimeMs);
    void setMemUsage(uint64_t resident,
                     uint64_t resident_max,
                     uint64_t virt,
                     uint64_t virt_max,
                     uint64_t total_phys,
                     uint64_t total_page);

private:
    void clearLocked();

    mutable android::base::Lock mLock;
    DISALLOW_COPY_AND_ASSIGN(StructuredInfo);
};

}  // crashreport
}  // android
