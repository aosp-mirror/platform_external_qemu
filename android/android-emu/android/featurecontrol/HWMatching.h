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

#include "android/opengl/gpuinfo.h"

#include <string>
#include <vector>

namespace emulator_features {
class EmulatorFeaturePattern;
class EmulatorFeaturePatterns;
}

namespace android {
namespace featurecontrol {

// These are only exposed in the header file
// for unit testing purposes.

struct FeatureAction {
    std::string name;
    bool enable;
};

struct HostHwInfo {
    std::string cpu_manufacturer;
    bool virt_support;
    bool running_in_vm;
    int os_bit_count;
    uint32_t cpu_model_name;
    std::string os_platform;
    const GpuInfoList* gpuinfolist;
};

HostHwInfo queryHostHwInfo();

bool matchFeaturePattern(
    const HostHwInfo& hostinfo,
    const emulator_features::EmulatorFeaturePattern* pattern);

std::vector<FeatureAction> matchFeaturePatterns(
    const HostHwInfo& hostinfo,
    const emulator_features::EmulatorFeaturePatterns* input);

}  // namespace featurecontrol
}  // namespace android
