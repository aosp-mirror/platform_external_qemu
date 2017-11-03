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

constexpr uint64_t secondsToNs(float seconds) {
    return static_cast<uint64_t>(seconds * 1000000000.0);
}

constexpr float nsToSeconds(uint64_t nanoSeconds) {
    return static_cast<float>(nanoSeconds / 1000000000.0);
}

/* Fixed state change time for smooth acceleration changes. */
constexpr float kTargetStateChangeTimeSeconds = 0.5f;
constexpr uint64_t kTargetStateChangeTimeNs =
        secondsToNs(kTargetStateChangeTimeSeconds);

InertialState InertialModel::setCurrentTime(uint64_t time_ns) {
    assert(time_ns >= mModelTimeNs);
    mModelTimeNs = time_ns;

    return mModelTimeNs >= mStateChangeEndTime ?
            INERTIAL_STATE_STABLE : INERTIAL_STATE_CHANGING;
}

void InertialModel::setTargetPosition(
        glm::vec3 position, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        // For Step changes, we simply set the transform to immediately take the
        // user to the given position and not reflect any movement-based
        // acceleration or velocity.
        mAccelPhasePositionTransform = glm::mat4x3(
                glm::vec3(), glm::vec3(), glm::vec3(), position);
        mAccelPhasePositionQuarticScale = glm::vec3();
        mAccelPhaseVelocityTransform = glm::mat4x3(0.f);
        mAccelPhaseAccelerationTransform = glm::mat4x3(0.f);

        mDecelPhasePositionTransform = glm::mat4x3(
                glm::vec3(), glm::vec3(), glm::vec3(), position);
        mDecelPhasePositionQuarticScale = glm::vec3();
        mDecelPhaseVelocityTransform = glm::mat4x3(0.f);
        mDecelPhaseAccelerationTransform = glm::mat4x3(0.f);

        mStateChangeStartTime = mModelTimeNs;
        mStateChangeEndTime = mModelTimeNs + kTargetStateChangeTimeNs;
    } else {
        // We ensure that velocity, acceleration, and position are continuously
        // interpolating from the current state.  Here, and throughout, x is the
        // position, v is the velocity, and a is the acceleration.
        const glm::vec3 x_init = getPosition();
        const glm::vec3 v_init = getVelocity();
        const glm::vec3 a_init = getAcceleration();
        const glm::vec3 x_target = position;

        // constexprs for powers of the state change time which are used
        // repeated.
        constexpr float time_1 = kTargetStateChangeTimeSeconds;
        constexpr float time_2 = time_1 * time_1;
        constexpr float time_3 = time_2 * time_1;
        constexpr float time_4 = time_2 * time_2;

        // Computed by solving for quartic movement in two phases, half of
        // kTargetStateChangeTimeSeconds spent in the first phase (here called
        // acceleration) and half spent in the second phase (deceleration).
        // Position, Velocity, and Acceleration are computed here by solving
        // the system of linear equations created by taking as a given that
        // these quartic equations start with acceleration, velocity, and
        // position equal to the current values at the start, and position equal
        // to the target value at the end, with zero velocity and acceleration.
        // At the half-way point where we transition from the accel and decel
        // phase, the two equations are required to be equal in through the 3rd
        // derivative.
        //
        // Equations of motion are:
        //
        //        [       0 <= t <= t_end/2 | f_0(t) ]
        // f(t) = [                         |        ]
        //        [ t_end/2 <= t <= t_end   | f_1(t) ]
        //
        // f_0(t) == At^4 + Bt^3 + Ct^2 + Dt + E
        //
        // f_1(t) == Ft^4 + Gt^3 + Ht^2 + It + J
        //
        // Where:
        //     A == accelQuarticTerm
        //     B == accelCubicTerm
        //     C == accelQuadraticTerm
        //     D == accelLinearTerm
        //     E == accelConstantTerm
        //     F == decelQuarticTerm
        //     G == decelCubicTerm
        //     H == decelQuadraticTerm
        //     I == decelLinearTerm
        //     J == decelConstantTerm
        // t_end == kTargetStateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f_0(0) == x_init
        //            df_0/d_t(0) == v_init
        //           d2f_0/d2t(0) == a_init
        // Transition State:
        //           f_0(t_end/2) == f_1(t_end/2)
        //       df_0/dt(t_end/2) == df_1/dt(t_end/2)
        //     d2f_0/d2t(t_end/2) == d2f_1/d2t(t_end/2)
        //     d3f_0/d3t(t_end/2) == d3f_1/d3t(t_end/2)
        // Final State:
        //             f_1(t_end) == x_target
        //         df_1/dt(t_end) == 0
        //       d2f_1/d2t(t_end) == 0

        const glm::vec3 accelQuarticTerm = (1.f / (12.f * time_4)) * (
                11.f * time_2 * a_init +
                54.f * time_1 * v_init +
                96.f * x_init +
                -96.f * x_target);
        const glm::vec3 accelCubicTerm = (1.f / (3.f * time_3)) * (
                -4.f * time_2 * a_init +
                -15.f * time_1 * v_init +
                -24.f * x_init +
                24.f * x_target);
        const glm::vec3 accelQuadraticTerm = (1.f / 2.f) * a_init;
        const glm::vec3 accelLinearTerm = v_init;
        const glm::vec3 accelConstantTerm = x_init;

        const glm::vec3 decelQuarticTerm = (1.f / (12.f * time_4)) * (
                -5.f * time_2 * a_init +
                -42.f * time_1 * v_init +
                -96.f * x_init +
                96.f * x_target);
        const glm::vec3 decelCubicTerm = (1.f / (3.f * time_3)) * (
                4.f * time_2 * a_init +
                33.f * time_1 * v_init +
                72.f * x_init +
                -72.f * x_target);
        const glm::vec3 decelQuadraticTerm = (-3.f / (2.f * time_2)) * (
                time_2 * a_init +
                8.f * time_1 * v_init +
                16.f * x_init +
                -16.f * x_target);
        const glm::vec3 decelLinearTerm = (1.f / (3.f * time_1)) * (
                2.f * time_2 * a_init +
                15.f * time_1 * v_init +
                24.f * x_init +
                -24.f * x_target);
        const glm::vec3 decelConstantTerm = (1.f / 12.f) * (
                -1.f * time_2 * a_init +
                -6.f * time_1 * v_init +
                12.f * x_target);

        mAccelPhasePositionTransform = glm::mat4x3(
                accelCubicTerm,
                accelQuadraticTerm,
                accelLinearTerm,
                accelConstantTerm);
        mAccelPhasePositionQuarticScale = accelQuarticTerm;
        mAccelPhaseVelocityTransform = glm::mat4x3(
                4.f * accelQuarticTerm,
                3.f * accelCubicTerm,
                2.f * accelQuadraticTerm,
                accelLinearTerm);
        mAccelPhaseAccelerationTransform = glm::mat4x3(
                glm::vec3(),
                12.f * accelQuarticTerm,
                6.f * accelCubicTerm,
                2.f * accelQuadraticTerm);

        mDecelPhasePositionTransform = glm::mat4x3(
                decelCubicTerm,
                decelQuadraticTerm,
                decelLinearTerm,
                decelConstantTerm);
        mDecelPhasePositionQuarticScale = decelQuarticTerm;
        mDecelPhaseVelocityTransform = glm::mat4x3(
                4.f * decelQuarticTerm,
                3.f * decelCubicTerm,
                2.f * decelQuadraticTerm,
                decelLinearTerm);
        mDecelPhaseAccelerationTransform = glm::mat4x3(
                glm::vec3(),
                12.f * decelQuarticTerm,
                6.f * decelCubicTerm,
                2.f * decelQuadraticTerm);

        mStateChangeStartTime = mModelTimeNs;
        mStateChangeEndTime = mModelTimeNs + kTargetStateChangeTimeNs;
    }
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
    return calculateInertialState(
        mAccelPhasePositionTransform,
        mDecelPhasePositionTransform,
        mAccelPhasePositionQuarticScale,
        mDecelPhasePositionQuarticScale);
}

glm::vec3 InertialModel::getVelocity() const {
    return calculateInertialState(
        mAccelPhaseVelocityTransform,
        mDecelPhaseVelocityTransform);
}

glm::vec3 InertialModel::getAcceleration() const {
    return calculateInertialState(
        mAccelPhaseAccelerationTransform,
        mDecelPhaseAccelerationTransform);
}

glm::quat InertialModel::getRotation() const {
    return mCurrentRotation;
}

glm::vec3 InertialModel::getRotationalVelocity() const {
    return mRotationalVelocity;
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

glm::vec3 InertialModel::calculateInertialState(
        const glm::mat4x3 accelPhaseTransform,
        const glm::mat4x3 decelPhaseTransform,
        const glm::vec3 accelPhaseQuarticScale,
        const glm::vec3 decelPhaseQuarticScale) const {
    assert(mModelTimeNs >= mStateChangeStartTime);
    const float time_1 = nsToSeconds(
            (mModelTimeNs < mStateChangeEndTime ?
                    mModelTimeNs : mStateChangeEndTime) -
            mStateChangeStartTime);
    const float time_2 = time_1 * time_1;
    const float time_3 = time_2 * time_1;
    const float time_4 = time_3 * time_1;
    glm::vec4 time_vec(time_3, time_2, time_1, 1.f);
    if (mModelTimeNs - mStateChangeStartTime <
            ((mStateChangeEndTime - mStateChangeStartTime) / 2UL)) {
        return accelPhaseTransform * time_vec +
                time_4 * accelPhaseQuarticScale;
    } else {
        return decelPhaseTransform * time_vec +
                time_4 * decelPhaseQuarticScale;
    }
}

}  // namespace physics
}  // namespace android
