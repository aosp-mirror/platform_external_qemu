#pragma once

#include <string>

namespace android {
namespace featurecontrol {

// These are for unit tests only.

struct FeatureAction {
    std::string name;
    bool shouldDisable;
    bool shouldEnable;
};

} // namespace featurecontrol
} // namespace android
