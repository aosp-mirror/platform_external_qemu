

// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/utils/ServiceUtils.h"

#include <unordered_map>
#include <string>

namespace android {
namespace emulation {
namespace control {

std::unordered_map<std::string, std::string> getQemuConfig(
        AndroidHwConfig* config) {
    std::unordered_map<std::string, std::string> cfg;

    /* use the magic of macros to implement the hardware configuration loaded */

#define HWCFG_BOOL(n, s, d, a, t) cfg[s] = config->n ? "true" : "false";
#define HWCFG_INT(n, s, d, a, t) cfg[s] = std::to_string(config->n);
#define HWCFG_STRING(n, s, d, a, t) cfg[s] = config->n;
#define HWCFG_DOUBLE(n, s, d, a, t) cfg[s] std::to_string(config->n);
#define HWCFG_DISKSIZE(n, s, d, a, t) cfg[s] = std::to_string(config->n);

#include "android/avd/hw-config-defs.h"

    return cfg;
}

}  // namespace control
}  // namespace emulation
}  // namespace android