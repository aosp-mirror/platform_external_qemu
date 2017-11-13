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

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

namespace android {
namespace physics {

constexpr uint64_t secondsToNs(float seconds) {
    return static_cast<uint64_t>(seconds * 1000000000.0);
}

constexpr float nsToSeconds(uint64_t nanoSeconds) {
    return static_cast<float>(nanoSeconds / 1000000000.0);
}

/* Fixed state change time for smooth acceleration changes. */
constexpr float kPositionStateChangeTimeSeconds = 0.5f;
constexpr uint64_t kPositionStateChangeTimeNs =
        secondsToNs(kPositionStateChangeTimeSeconds);

/* maximum angular velocity in rad/s */
constexpr float kMaxAngularVelocity = 5.f;
constexpr float kTargetRotationTime = 0.050f;

InertialState InertialModel::setCurrentTime(uint64_t time_ns) {
    assert(time_ns >= mModelTimeNs);
    mModelTimeNs = time_ns;

    return (mModelTimeNs >= mPositionChangeEndTime &&
            mModelTimeNs >= mRotationChangeEndTime) ?
            INERTIAL_STATE_STABLE : INERTIAL_STATE_CHANGING;
}

void InertialModel::setTargetPosition(
        glm::vec3 position, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        // For Step changes, we simply set the transform to immediately take the
        // user to the given position and not reflect any movement-based
        // acceleration or velocity.
        mPositionCubic = glm::mat4x3(
                glm::vec3(), glm::vec3(), glm::vec3(), position);
        mPositionQuintic = glm::mat2x3(0.f);
        mVelocityCubic = glm::mat4x3(0.f);
        mVelocityQuintic = glm::mat2x3(0.f);
        mAccelerationCubic = glm::mat4x3(0.f);
        mAccelerationQuintic = glm::mat2x3(0.f);

        mPositionChangeStartTime = mModelTimeNs;
        mPositionChangeEndTime = mModelTimeNs + kPositionStateChangeTimeNs;
    } else {
        // We ensure that velocity, acceleration, and position are continuously
        // interpolating from the current state.  Here, and throughout, x is the
        // position, v is the velocity, and a is the acceleration.
        const glm::vec3 x_init = getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 v_init = getVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 a_init = getAcceleration(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 x_target = position;

        // constexprs for powers of the state change time which are used
        // repeated.
        constexpr float time_1 = kPositionStateChangeTimeSeconds;
        constexpr float time_2 = time_1 * time_1;
        constexpr float time_3 = time_2 * time_1;
        constexpr float time_4 = time_2 * time_2;
        constexpr float time_5 = time_2 * time_3;

        // Computed by solving for quintic movement in
        // kPositionStateChangeTimeSeconds. Position, Velocity, and Acceleration
        // are computed here by solving the system of linear equations created
        // by setting the initial position, velocity, and acceleration to the
        // current values, and the final state to the target position, with no
        // velocity or acceleration.
        //
        // Equation of motion is:
        //
        // f(t) == At^5 + Bt^4 + Ct^3 + Dt^2 + Et + F
        //
        // Where:
        //     A == quinticTerm
        //     B == quarticTerm
        //     C == cubicTerm
        //     D == quadraticTerm
        //     E == linearTerm
        //     F == constantTerm
        // t_end == kPositionStateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f(0) == x_init
        //            df/d_t(0) == v_init
        //           d2f/d2t(0) == a_init
        // Final State:
        //             f(t_end) == x_target
        //         df/dt(t_end) == 0
        //       d2f/d2t(t_end) == 0
        //
        // These can be solved via the following Mathematica command:
        // RowReduce[{{0,0,0,0,0,1,x},
        //            {0,0,0,0,1,0,v},
        //            {0,0,0,2,0,0,a},
        //            {t^5,t^4,t^3,t^2,t,1,y},
        //            {5t^4,4t^3,3t^2,2t,1,0,0},
        //            {20t^3,12t^2,6t,2,0,0,0}}]
        //
        // Where:
        //     x = x_init
        //     v = v_init
        //     a = a_init
        //     t = kPositionStateChangeTimeSeconds
        //     y = x_target

        const glm::vec3 quinticTerm = (1.f / (2.f * time_5)) * (
                -1.f * time_2 * a_init +
                -6.f * time_1 * v_init +
                -12.f * x_init +
                12.f * x_target);

        const glm::vec3 quarticTerm = (1.f / (2.f * time_4)) * (
                3.f * time_2 * a_init +
                16.f * time_1 * v_init +
                30.f * x_init +
                -30.f * x_target);

        const glm::vec3 cubicTerm = (1.f / (2.f * time_3)) * (
                -3.f * time_2 * a_init +
                -12.f * time_1 * v_init +
                -20.f * x_init +
                20.f * x_target);

        const glm::vec3 quadraticTerm = (1.f / 2.f) * a_init;

        const glm::vec3 linearTerm = v_init;

        const glm::vec3 constantTerm = x_init;

        mPositionQuintic = glm::mat2x3(
                quinticTerm,
                quarticTerm);
        mPositionCubic = glm::mat4x3(
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);

        mVelocityQuintic = glm::mat2x3(
                glm::vec3(0.0f),
                5.f * quinticTerm);
        mVelocityCubic = glm::mat4x3(
                4.f * quarticTerm,
                3.f * cubicTerm,
                2.f * quadraticTerm,
                linearTerm);

        mAccelerationQuintic = glm::mat2x3(
                glm::vec3(0.0f),
                glm::vec3(0.0f));
        mAccelerationCubic = glm::mat4x3(
                20.f * quinticTerm,
                12.f * quarticTerm,
                6.f * cubicTerm,
                2.f * quadraticTerm);

        mPositionChangeStartTime = mModelTimeNs;
        mPositionChangeEndTime = mModelTimeNs + kPositionStateChangeTimeNs;
    }
}

void InertialModel::setTargetRotation(
        glm::quat rotation, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        mInitialRotation = rotation;
    } else {
        mInitialRotation = getRotation(PARAMETER_VALUE_TYPE_CURRENT);
    }
    mRotationChangeStartTime = mModelTimeNs;

    const glm::quat rotationDiff = glm::normalize(
            rotation * glm::conjugate(mInitialRotation));
    mRotationAngleRadians = glm::angle(rotationDiff);
    mRotationAxis = glm::axis(rotationDiff);

    const float minTime = mRotationAngleRadians / kMaxAngularVelocity;
    mRotationTimeSeconds = std::max(kTargetRotationTime, minTime);
    mRotationChangeEndTime = mRotationChangeStartTime +
            secondsToNs(mRotationTimeSeconds);
}

glm::vec3 InertialModel::getPosition(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mPositionQuintic,
        mPositionCubic,
        parameterValueType);
}

glm::vec3 InertialModel::getVelocity(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mVelocityQuintic,
        mVelocityCubic,
        parameterValueType);
}

glm::vec3 InertialModel::getAcceleration(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mAccelerationQuintic,
        mAccelerationCubic,
        parameterValueType);
}

glm::quat InertialModel::getRotation(
        ParameterValueType parameterValueType) const {
    float changeFraction = 1.f;
    if (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT &&
            mModelTimeNs < mRotationChangeEndTime) {
        changeFraction = nsToSeconds(mModelTimeNs - mRotationChangeStartTime) /
                mRotationTimeSeconds;
    }
    return glm::normalize(glm::angleAxis(mRotationAngleRadians * changeFraction,
            mRotationAxis) * mInitialRotation);
}

glm::vec3 InertialModel::getRotationalVelocity(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT &&
            mModelTimeNs < mRotationChangeEndTime) {
        return (mRotationAngleRadians / mRotationTimeSeconds) * mRotationAxis;
    } else {
        return glm::vec3();
    }
}

glm::vec3 InertialModel::calculateInertialState(
            const glm::mat2x3& quinticTransform,
            const glm::mat4x3& cubicTransform,
            ParameterValueType parameterValueType) const {
    assert(mModelTimeNs >= mPositionChangeStartTime);
    const uint64_t requestedTimeNs =
            (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT &&
            mModelTimeNs < mPositionChangeEndTime) ?
                    mModelTimeNs : mPositionChangeEndTime;
    const float time1 = nsToSeconds(requestedTimeNs - mPositionChangeStartTime);
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;
    const float time4 = time2 * time2;
    const float time5 = time2 * time3;
    glm::vec2 quinticTimeVec(time5, time4);
    glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);
    return cubicTransform * cubicTimeVec +
            quinticTransform * quinticTimeVec;
}

}  // namespace physics
}  // namespace android
