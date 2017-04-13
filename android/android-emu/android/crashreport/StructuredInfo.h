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
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/HostHwInfo.h"
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
    void addHostHwInfo(const HostHwInfo::Info& info);
    void addGuestGlInfo(const char* vendor,
                        const char* renderer,
                        const char* version);
    void addUptime(uint64_t uptimeMs);

private:
    void clearLocked();

    mutable android::base::Lock mLock;
    DISALLOW_COPY_AND_ASSIGN(StructuredInfo);
};

}  // crashreport
}  // android
