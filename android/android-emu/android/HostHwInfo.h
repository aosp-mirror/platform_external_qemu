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

#include "android/base/Compiler.h"
#include "android/opengl/gpuinfo.h"

namespace android {

class HostHwInfo {
public:
    HostHwInfo();

    struct Info {
        std::string cpu_manufacturer;
        bool virt_support;
        bool running_in_vm;
        int16_t os_bit_count;
        uint32_t cpu_model_name;
        std::string os_platform;
        const GpuInfoList* gpuinfolist;
    };

    static const Info& query();

    struct ArmCpuInfo {
        // if true, it has at least two different cpu models
        // e.g., cpu0-3: cortex-a53 (0xd03)
        // cpu4-5: cortex-a72 (0xd08)
        bool is_big_little = false;
        // model id of cpus, the order matters
        std::vector<int> cpumodels;
    };

    static const ArmCpuInfo& queryArmCpuInfo();

private:
    Info info;
    ArmCpuInfo armCpuInfo;
    DISALLOW_COPY_AND_ASSIGN(HostHwInfo);
};

} // namespace android
