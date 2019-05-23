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
#include "android/base/CpuTime.h"

namespace android {
namespace base {

CpuTime operator-(const CpuTime& a, const CpuTime& other) {
    CpuTime res = a;
    res -= other;
    return res;
}

CpuTime& CpuTime::operator-=(const CpuTime& other) {
    wall_time_us -= other.wall_time_us;
    user_time_us -= other.user_time_us;
    system_time_us -= other.system_time_us;
    return *this;
}

uint64_t CpuTime::usageUs() const {
    return user_time_us + system_time_us;
}

float CpuTime::usage() const {
    if (!wall_time_us) return 0.0f;
    return (float)(user_time_us + system_time_us) / (float)(wall_time_us);
}

float CpuTime::usageUser() const {
    if (!wall_time_us) return 0.0f;
    return (float)(user_time_us) / (float)(wall_time_us);
}

float CpuTime::usageSystem() const {
    if (!wall_time_us) return 0.0f;
    return (float)(system_time_us) / (float)(wall_time_us);
}

} // namespace android
} // namespace base
