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

constexpr float kStateChangeTime1 = kPositionStateChangeTimeSeconds;
constexpr float kStateChangeTime2 = kStateChangeTime1 * kStateChangeTime1;
constexpr float kStateChangeTime3 = kStateChangeTime2 * kStateChangeTime1;
constexpr float kStateChangeTime4 = kStateChangeTime2 * kStateChangeTime2;
constexpr float kStateChangeTime5 = kStateChangeTime2 * kStateChangeTime3;

const glm::vec2 kQuinticTimeVec = glm::vec2(
        kStateChangeTime5, kStateChangeTime4);
const glm::vec4 kCubicTimeVec = glm::vec4(
        kStateChangeTime3, kStateChangeTime2, kStateChangeTime1, 1.f);

/* maximum angular velocity in rad/s */
constexpr float kMaxAngularVelocity = 5.f;
constexpr float kTargetRotationTime = 0.050f;

InertialState InertialModel::setCurrentTime(uint64_t time_ns) {
    assert(time_ns >= mModelTimeNs);
    mModelTimeNs = time_ns;

    return (mZeroVelocityAfterEndTime &&
            mModelTimeNs >= mPositionChangeEndTime &&
            mModelTimeNs >= mRotationChangeEndTime) ?
            INERTIAL_STATE_STABLE : INERTIAL_STATE_CHANGING;
}

void InertialModel::setTargetPosition(
        glm::vec3 position, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        // For Step changes, we simply set the transform to immediately take the
        // user to the given position and not reflect any movement-based
        // acceleration or velocity.
        setInertialTransforms(
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                position);
    } else {
        // We ensure that velocity, acceleration, and position are continuously
        // interpolating from the current state.  Here, and throughout, x is the
        // position, v is the velocity, and a is the acceleration.
        const glm::vec3 x_init = getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 v_init = getVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 a_init = getAcceleration(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 x_target = position;

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

        const glm::vec3 quinticTerm = (1.f / (2.f * kStateChangeTime5)) * (
                -1.f * kStateChangeTime2 * a_init +
                -6.f * kStateChangeTime1 * v_init +
                -12.f * x_init +
                12.f * x_target);

        const glm::vec3 quarticTerm = (1.f / (2.f * kStateChangeTime4)) * (
                3.f * kStateChangeTime2 * a_init +
                16.f * kStateChangeTime1 * v_init +
                30.f * x_init +
                -30.f * x_target);

        const glm::vec3 cubicTerm = (1.f / (2.f * kStateChangeTime3)) * (
                -3.f * kStateChangeTime2 * a_init +
                -12.f * kStateChangeTime1 * v_init +
                -20.f * x_init +
                20.f * x_target);

        const glm::vec3 quadraticTerm = (1.f / 2.f) * a_init;

        const glm::vec3 linearTerm = v_init;

        const glm::vec3 constantTerm = x_init;

        setInertialTransforms(
                quinticTerm,
                quarticTerm,
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);
    }
    mPositionChangeStartTime = mModelTimeNs;
    mPositionChangeEndTime = mModelTimeNs + kPositionStateChangeTimeNs;
    mZeroVelocityAfterEndTime = true;
}

void InertialModel::setTargetVelocity(
        glm::vec3 velocity, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        // For Step changes, we simply set the transform to immediately move the
        // user at a given velocity starting from the current position.
        setInertialTransforms(
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                velocity,
                getPosition(PARAMETER_VALUE_TYPE_CURRENT));
    } else {
        // We ensure that velocity, acceleration, and position are continuously
        // interpolating from the current state.  Here, and throughout, x is the
        // position, v is the velocity, and a is the acceleration.
        const glm::vec3 x_init = getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 v_init = getVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 a_init = getAcceleration(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::vec3 v_target = velocity;

        // Computed by solving for quartic movement in
        // kPositionStateChangeTimeSeconds. Position, Velocity, and Acceleration
        // are computed here by solving the system of linear equations created
        // by setting the initial position, velocity, and acceleration to the
        // current values, and the final state to the target velocity, with no
        // acceleration.  Note that there is no target position specified.
        //
        // Equation of motion is:
        //
        // f(t) == At^4 + Bt^3 + Ct^2 + Dt + E
        //
        // Where:
        //     A == quarticTerm
        //     B == cubicTerm
        //     C == quadraticTerm
        //     D == linearTerm
        //     E == constantTerm
        // t_end == kPositionStateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f(0) == x_init
        //            df/d_t(0) == v_init
        //           d2f/d2t(0) == a_init
        // Final State:
        //         df/dt(t_end) == v_target
        //       d2f/d2t(t_end) == 0
        //
        // These can be solved via the following Mathematica command:
        // RowReduce[{{0,0,0,0,1,x},
        //            {0,0,0,1,0,v},
        //            {0,0,2,0,0,a},
        //            {4t^3,3t^2,2t,1,0,w},
        //            {12t^2,6t,2,0,0,0}}]
        //
        // Where:
        //     x = x_init
        //     v = v_init
        //     a = a_init
        //     t = kPositionStateChangeTimeSeconds
        //     w = v_target

        const glm::vec3 quarticTerm = (1.f / (4.f * kStateChangeTime3)) * (
                1.f * kStateChangeTime1 * a_init +
                2.f * v_init +
                -2.f * v_target);
        const glm::vec3 cubicTerm = (1.f / (3.f * kStateChangeTime2)) * (
                -2.f * kStateChangeTime1 * a_init +
                -3.f * v_init +
                3.f * v_target);
        const glm::vec3 quadraticTerm = (1.f / 2.f) * a_init;
        const glm::vec3 linearTerm = v_init;
        const glm::vec3 constantTerm = x_init;

        setInertialTransforms(
                glm::vec3(0.0f),
                quarticTerm,
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);
    }
    mPositionChangeStartTime = mModelTimeNs;
    mPositionChangeEndTime = mModelTimeNs + kPositionStateChangeTimeNs;
    mZeroVelocityAfterEndTime = false;
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
    if (mRotationAngleRadians > M_PI) {
        mRotationAngleRadians -= 2.f * M_PI;
    }
    mRotationAxis = glm::axis(rotationDiff);

    const float minTime = std::abs(mRotationAngleRadians) / kMaxAngularVelocity;
    mRotationTimeSeconds = std::max(kTargetRotationTime, minTime);
    mRotationChangeEndTime = mRotationChangeStartTime +
            secondsToNs(mRotationTimeSeconds);
}

glm::vec3 InertialModel::getPosition(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mPositionQuintic,
        mPositionCubic,
        mPositionAfterEndCubic,
        parameterValueType);
}

glm::vec3 InertialModel::getVelocity(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mVelocityQuintic,
        mVelocityCubic,
        mVelocityAfterEndCubic,
        parameterValueType);
}

glm::vec3 InertialModel::getAcceleration(
        ParameterValueType parameterValueType) const {
    return calculateInertialState(
        mAccelerationQuintic,
        mAccelerationCubic,
        glm::mat4x3(0.f),
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

void InertialModel::setInertialTransforms(
        const glm::vec3 quinticCoefficient,
        const glm::vec3 quarticCoefficient,
        const glm::vec3 cubicCoefficient,
        const glm::vec3 quadraticCoefficient,
        const glm::vec3 linearCoefficient,
        const glm::vec3 constantCoefficient) {
    mPositionQuintic = glm::mat2x3(
            quinticCoefficient,
            quarticCoefficient);
    mPositionCubic = glm::mat4x3(
            cubicCoefficient,
            quadraticCoefficient,
            linearCoefficient,
            constantCoefficient);

    mVelocityQuintic = glm::mat2x3(
            glm::vec3(0.0f),
            5.f * quinticCoefficient);
    mVelocityCubic = glm::mat4x3(
            4.f * quarticCoefficient,
            3.f * cubicCoefficient,
            2.f * quadraticCoefficient,
            linearCoefficient);

    mAccelerationQuintic = glm::mat2x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f));
    mAccelerationCubic = glm::mat4x3(
            20.f * quinticCoefficient,
            12.f * quarticCoefficient,
            6.f * cubicCoefficient,
            2.f * quadraticCoefficient);

    glm::vec3 endPosition = mPositionCubic * kCubicTimeVec +
            mPositionQuintic * kQuinticTimeVec;
    glm::vec3 endVelocity = mVelocityCubic * kCubicTimeVec +
            mVelocityQuintic * kQuinticTimeVec;

    mPositionAfterEndCubic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            endVelocity,
            endPosition);
    mVelocityAfterEndCubic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            endVelocity);
}

glm::vec3 InertialModel::calculateInertialState(
        const glm::mat2x3& quinticTransform,
        const glm::mat4x3& cubicTransform,
        const glm::mat4x3& afterEndCubicTransform,
        ParameterValueType parameterValueType) const {
    assert(mModelTimeNs >= mPositionChangeStartTime);
    const uint64_t requestedTimeNs =
            parameterValueType == PARAMETER_VALUE_TYPE_TARGET ?
                    mPositionChangeEndTime : mModelTimeNs;

    const float time1 = nsToSeconds(requestedTimeNs - mPositionChangeStartTime);
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;
    const float time4 = time2 * time2;
    const float time5 = time2 * time3;
    glm::vec2 quinticTimeVec(time5, time4);
    glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);

    return requestedTimeNs < mPositionChangeEndTime ?
            cubicTransform * cubicTimeVec + quinticTransform * quinticTimeVec :
            afterEndCubicTransform * cubicTimeVec;
}

}  // namespace physics
}  // namespace android
