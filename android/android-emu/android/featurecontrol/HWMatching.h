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
    bool shouldDisable;
    bool shouldEnable;
};

struct HostHwInfo {
    std::string cpu_manufacturer;
    bool virt_support;
    bool running_in_vm;
    int os_bit_count;
    uint32_t cpu_model_name;
    std::string os_platform;
    GpuInfoList* gpuinfolist;
};

HostHwInfo queryHostHwInfo();

bool matchHostHwAgainstFeaturePattern(
        const HostHwInfo& hostinfo,
        const emulator_features::EmulatorFeaturePattern* pattern);

std::vector<FeatureAction> matchHostHwAgainstFeaturePatterns(
        const HostHwInfo& hostinfo,
        const emulator_features::EmulatorFeaturePatterns* input);

} // namespace featurecontrol
} // namespace android
