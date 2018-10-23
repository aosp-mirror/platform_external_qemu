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

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <array>

#include "android/physics/Physics.h"

namespace android {
namespace physics {

typedef enum {
    AMBIENT_STATE_CHANGING=0,
    AMBIENT_STATE_STABLE=1,
} AmbientState;

/*
 * Implements a model of an ambient environment, within which sensor
 * values could be calculated related to the ambient state (e.g. the
 * ambient gravity vector, magnetic vector, etc...).
 *
 * The ambient environment should be created, and its attribute targets
 * should be set as desired.  When sensor values are to be calculated
 * and require an ambient attribute as input, that attribute should be
 * polled from the ambient environment to get the most recent state.
 */
class AmbientEnvironment {
public:
    AmbientEnvironment() = default;

    /*
     * Sets the current time of the AmbientEnvironment simulation.  This time is
     * used as the current time in calculating ambient environment states, along
     * with the time when target state change requests are recorded as taking
     * place.  Time values must be non-decreasing.
     */
    AmbientState setCurrentTime(uint64_t time_ns);

    /*
     * Sets the strength of the ambient magnetic field.
     */
    void setMagneticField(
        float north, float east, float vertical, PhysicalInterpolation mode);

    /*
     * Sets the ambient gravity vector.
     */
    void setGravity(glm::vec3 gravity, PhysicalInterpolation mode);

    /*
     * Sets the ambient temperature.
     */
    void setTemperature(float celsius, PhysicalInterpolation mode);

    /*
     * Sets the target proximity value.
     */
    void setProximity(float centimeters, PhysicalInterpolation mode);

    /*
     * Sets the target ambient light value.
     */
    void setLight(float lux, PhysicalInterpolation mode);

    /*
     * Sets the target barometric pressure value.
     */
    void setPressure(float hPa, PhysicalInterpolation mode);

    /*
     * Sets the target humidity value.
     */
    void setHumidity(float percent, PhysicalInterpolation mode);

    /*
     * Gets current simulated state of the ambient environment.
     */
    glm::vec3 getMagneticField(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    glm::vec3 getGravity(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    float getTemperature(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    float getProximity(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    float getLight(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    float getPressure(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;
    float getHumidity(
            ParameterValueType parameterValueType = PARAMETER_VALUE_TYPE_CURRENT) const;

private:
    static constexpr glm::vec3 kDefaultMagneticField =
            glm::vec3(0.0f, 5.9f, -48.4f);
    static constexpr glm::vec3 kDefaultGravity = glm::vec3(0.f, -9.81f, 0.f);

    /* celsius */
    static constexpr float kDefaultTemperature = 0.f;
    /* cm */
    static constexpr float kDefaultProximity = 1.f;
    /* lux */
    static constexpr float kDefaultLight = 0.f;
    /* hPa */
    static constexpr float kDefaultPressure = 0.f;
    /* percent */
    static constexpr float kDefaultHumidity = 0.f;

    glm::vec3 mMagneticField = kDefaultMagneticField;
    glm::vec3 mGravity = kDefaultGravity;
    float mTemperature = kDefaultTemperature;
    float mProximity = kDefaultProximity;
    float mLight = kDefaultLight;
    float mPressure = kDefaultPressure;
    float mHumidity = kDefaultHumidity;
};

}  // namespace physics
}  // namespace android
