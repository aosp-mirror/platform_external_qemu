/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>

#include "android/physics/Physics.h"

namespace android {
namespace physics {

enum class BodyState {
    CHANGING=0,
    STABLE=1,
};

class BodyModel {
public:
    BodyModel() = default;

    /*
     * Sets the current time of the AmbientEnvironment simulation.  This time is
     * used as the current time in calculating body states, along
     * with the time when target state change requests are recorded as taking
     * place.  Time values must be non-decreasing.
     */
    BodyState setCurrentTime(uint64_t time_ns);

    /*
     * Sets the body heart rate.
     */
    void setHeartRate(float bpm, PhysicalInterpolation mode);

    float getHeartRate(
        ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;

 private:
    /* BPM */
    static constexpr float kDefaultHeartRate = 0.f;

    float mHeartRate = kDefaultHeartRate;
};
} // namespace physics
} // namespace android
