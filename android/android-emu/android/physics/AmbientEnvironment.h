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
    glm::vec3 getMagneticField() const;
    glm::vec3 getGravity() const;
    float getTemperature() const;
    float getProximity() const;
    float getLight() const;
    float getPressure() const;
    float getHumidity() const;

private:
    glm::vec3 mMagneticField = glm::vec3(22.0f, 5.9f, 43.1f);
    glm::vec3 mGravity = glm::vec3(0.f, -9.81f, 0.f);

    /* celsius */
    float mTemperature = 0.f;
    /* cm */
    float mProximity = 1.f;
    /* lux */
    float mLight = 0.f;
    /* hPa */
    float mPressure = 0.f;
    /* percent */
    float mHumidity = 0.f;
};

}  // namespace physics
}  // namespace android
