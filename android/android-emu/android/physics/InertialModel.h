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
 * Implements a model of inertial motion of a rigid body such that smooth
 * movement occurs bringing the body to the target rotation and position with
 * dependent sensor data constantly available and up to date.
 *
 * The inertial model should be used by sending it target positions and
 * then polling the current actual rotation and position, acceleration and
 * velocity values in order to find the current state of the rigid body.
 */
class InertialModel {
public:
    InertialModel() = default;

    /*
     * Sets the position that the modeled object should move toward.
     */
    void setTargetPosition(glm::vec3 position, PhysicalInterpolation mode);

    /*
     * Sets the rotation that the modeled object should move toward.
     */
    void setTargetRotation(glm::quat rotation, PhysicalInterpolation mode);

    /*
     * Gets current simulated state and sensor values of the modeled object.
     */
    glm::vec3 getPosition() const;
    glm::vec3 getAcceleration() const;
    glm::quat getRotation() const;
    glm::vec3 getRotationalVelocity() const;

private:
    void updateAccelerations();
    void updateRotations();

    glm::vec3 mAcceleration = glm::vec3(0.f);
    glm::vec3 mPreviousPosition = glm::vec3(0.f);
    glm::vec3 mCurrentPosition = glm::vec3(0.f);
    glm::quat mPreviousRotation;
    glm::quat mCurrentRotation;
    glm::vec3 mRotationalVelocity = glm::vec3(0.f);

    uint64_t mUpdateIntervalMs = 0;
    uint64_t mLastRotationUpdateMs = 0;
    static const uint64_t ROTATION_UPDATE_RESET_TIME_MS = 100;
    static const size_t ROTATION_UPDATE_TIME_WINDOW_SIZE = 8;
    std::array<uint64_t, ROTATION_UPDATE_TIME_WINDOW_SIZE>
        mLastUpdateIntervals = {};
    size_t mRotationUpdateTimeWindowElt = 0;
};

}  // namespace physics
}  // namespace android
