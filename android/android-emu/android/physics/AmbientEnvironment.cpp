/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "android/physics/AmbientEnvironment.h"

namespace android {
namespace physics {

AmbientState AmbientEnvironment::setCurrentTime(uint64_t time_ns) {
    mCurrentTimeNs = time_ns;
    return AMBIENT_STATE_STABLE;
}

void AmbientEnvironment::setMagneticField(
        float north, float east, float vertical, PhysicalInterpolation mode) {
    mMagneticField = glm::vec3(north, east, vertical);
}

void AmbientEnvironment::setGravity(
        glm::vec3 gravity, PhysicalInterpolation mode) {
    mGravity = gravity;
}

void AmbientEnvironment::setTemperature(
        float celsius, PhysicalInterpolation mode) {
    mTemperature = celsius;
}

void AmbientEnvironment::setProximity(
        float centimeters, PhysicalInterpolation mode) {
    mProximity = centimeters;
}

void AmbientEnvironment::setLight(float lux, PhysicalInterpolation mode) {
    mLight = lux;
}

void AmbientEnvironment::setPressure(float hPa, PhysicalInterpolation mode) {
    mPressure = hPa;
}

void AmbientEnvironment::setHumidity(
        float percent, PhysicalInterpolation mode) {
    mHumidity = percent;
}

glm::vec3 AmbientEnvironment::getMagneticField(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mMagneticField;
}

glm::vec3 AmbientEnvironment::getGravity(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mGravity;
}

float AmbientEnvironment::getTemperature(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mTemperature;
}

float AmbientEnvironment::getProximity(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mProximity;
}

float AmbientEnvironment::getLight(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mLight;
}

float AmbientEnvironment::getPressure(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mPressure;
}

float AmbientEnvironment::getHumidity(
        ParameterValueType valueType,
        uint64_t* timestamp) const {
    if (timestamp != nullptr) {
        *timestamp = mCurrentTimeNs;
    }
    return mHumidity;
}

}  // namespace physics
}  // namespace android
