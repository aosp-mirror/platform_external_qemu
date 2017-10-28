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

#include "android/physics/InertialModel.h"

#include "android/base/system/System.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"

namespace android {
namespace physics {

void InertialModel::setTargetPosition(
        glm::vec3 position, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        mPreviousPosition = position;
    }
    mCurrentPosition = position;
    updateAccelerations();
}

void InertialModel::setTargetRotation(
        glm::quat rotation, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        mPreviousRotation = rotation;
    } else {
        mPreviousRotation = mCurrentRotation;
    }
    mCurrentRotation = rotation;
    updateRotations();
}

glm::vec3 InertialModel::getPosition() const {
    return mCurrentPosition;
}

glm::vec3 InertialModel::getAcceleration() const {
    return mAcceleration;
}

glm::quat InertialModel::getRotation() const {
    return mCurrentRotation;
}

glm::vec3 InertialModel::getRotationalVelocity() const {
    return mRotationalVelocity;
}

void InertialModel::updateAccelerations() {
    constexpr float k = 100.f;
    constexpr float mass = 1.f;
    constexpr float meters_per_unit = 0.0254f;

    const glm::vec3 delta = (meters_per_unit * (mCurrentPosition - mPreviousPosition));
    mAcceleration = delta * k / mass;
    mPreviousPosition = mCurrentPosition;
}

void InertialModel::updateRotations() {
    const uint64_t currTimeMs = static_cast<uint64_t>(
        android::base::System::get()->getHighResTimeUs() / 1000);

    if (currTimeMs - mLastRotationUpdateMs > ROTATION_UPDATE_RESET_TIME_MS) {
        std::fill(mLastUpdateIntervals.begin(),
                  mLastUpdateIntervals.end(), 0);
    }

    mLastUpdateIntervals[mRotationUpdateTimeWindowElt] =
        (currTimeMs - mLastRotationUpdateMs);
    mRotationUpdateTimeWindowElt++;

    if (mRotationUpdateTimeWindowElt >= ROTATION_UPDATE_TIME_WINDOW_SIZE) {
        mRotationUpdateTimeWindowElt = 0;
    }

    mUpdateIntervalMs = 0;

    // Filter out window entries where the update interval is
    // still calculated as 0 due to not being initialized yet.
    uint64_t nontrivialUpdateIntervals = 0;
    for (const auto& elt: mLastUpdateIntervals) {
        mUpdateIntervalMs += elt;
        if (elt) nontrivialUpdateIntervals++;
    }

    if (nontrivialUpdateIntervals)
        mUpdateIntervalMs /= nontrivialUpdateIntervals;

    mLastRotationUpdateMs = currTimeMs;

    const glm::quat rotationDelta =
            mCurrentRotation * glm::conjugate(mPreviousRotation);
    const glm::vec3 eulerAngles = glm::eulerAngles(rotationDelta);

    const float gyroUpdateRate = mUpdateIntervalMs < 16 ? 0.016f :
            mUpdateIntervalMs / 1000.0f;

    // Convert raw UI pitch/roll/yaw delta
    // to device-space rotation in radians per second.
    mRotationalVelocity =
            (eulerAngles * static_cast<float>(M_PI) / (180.f * gyroUpdateRate));
}

}  // namespace physics
}  // namespace android
