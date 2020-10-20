// Copyright 2018 The Android Open Source Project
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

#include <inttypes.h>

namespace android {
namespace base {

class Looper;

// CPU usage tracking
struct CpuTime {
    uint64_t wall_time_us = 0;
    uint64_t user_time_us = 0;
    uint64_t system_time_us = 0;

    CpuTime& operator-=(const CpuTime& other);

    uint64_t usageUs() const;
    // Usage proportion; convert to percent by
    // multiplying by 100.
    float usage() const;
    float usageUser() const;
    float usageSystem() const;
};

CpuTime operator-(const CpuTime& a, const CpuTime& b);

} // namespace base
} // namespace android
