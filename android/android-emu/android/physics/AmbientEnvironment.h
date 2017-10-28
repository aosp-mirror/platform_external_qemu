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
        float north, float east, float vertical, bool instantaneous);

    /*
     * Sets the ambient gravity vector.
     */
    void setGravity(glm::vec3 gravity);

    /*
     * Gets current simulated state and sensor values of the modeled object.
     */
    glm::vec3 getMagneticField() const;
    glm::vec3 getGravity() const;

private:
    glm::vec3 mMagneticField = glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 mGravity = glm::vec3(0.f, -9.81f, 0.f);
};

}  // namespace physics
}  // namespace android
