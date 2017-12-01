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

/* Fixed state change time for smooth acceleration changes. */
constexpr uint64_t kStateChangeTimeNs = secondsToNs(kStateChangeTimeSeconds);

constexpr float kStateChangeTime1 = kStateChangeTimeSeconds;
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
        // kStateChangeTimeSeconds. Position, Velocity, and Acceleration
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
        // t_end == kStateChangeTimeSeconds
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
        //     t = kStateChangeTimeSeconds
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
    mPositionChangeEndTime = mModelTimeNs + kStateChangeTimeNs;
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
        // kStateChangeTimeSeconds. Position, Velocity, and Acceleration
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
        // t_end == kStateChangeTimeSeconds
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
        //     t = kStateChangeTimeSeconds
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
    mPositionChangeEndTime = mModelTimeNs + kStateChangeTimeNs;
    mZeroVelocityAfterEndTime = false;
}

void InertialModel::setTargetRotation(
        glm::quat rotation, PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        // For Step changes, we simply set the transform to immediately set the
        // rotation to the target, with zero rotational velocity.
        mRotationCubic = glm::mat4x4(
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
        mRotationalVelocityCubic = glm::mat4x4(0.f);
    } else {
        // Computed by solving for cubic movement in 4d space. Position and
        // Velocity in 4d space are computed here by solving the system of
        // linear equations created by setting the initial 4d position (i.e.
        // rotation), and velocity (i.e. rotational velocity) to the current
        // normalized values, and the final state to the target 4d position,
        // with zero velocity.
        //
        // Equation of motion is:
        //
        // f(t) == At^3 + Bt^2 + Ct + D
        //
        // Where:
        //     A == cubicTerm
        //     B == quadraticTerm
        //     C == linearTerm
        //     D == constantTerm
        // t_end == kRotationStateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f(0) == x_init
        //            df/d_t(0) == v_init
        // Final State:
        //             f(t_end) == x_target
        //         df/dt(t_end) == 0
        //
        // These can be solved via the following Mathematica command:
        // RowReduce[{{0,0,0,1,x},
        //            {0,0,1,0,v},
        //            {t^3,t^2,t,1,y},
        //            {3*t^2,2t,1,0,0}}]
        //
        // Where:
        //     x = x_init
        //     v = v_init
        //     t = kStateChangeTimeSeconds
        //     y = x_target

        const glm::vec4 currentRotation = calculateRotationalState(
                mRotationCubic,
                PARAMETER_VALUE_TYPE_CURRENT);

        const glm::vec4 currentRotationalVelocity = calculateRotationalState(
                mRotationalVelocityCubic,
                PARAMETER_VALUE_TYPE_CURRENT);

        const float rotationLength = glm::length(currentRotation);

        // Rotation length should not be zero, but it may be possible by driving
        // the inertial model in an extreme way (i.e. well timed oscilations) to
        // hit this case.  In this case, we will simply do a step to the target.
        if (rotationLength == 0.f) {
            mRotationCubic = glm::mat4x4(
                    glm::vec4(0.f),
                    glm::vec4(0.f),
                    glm::vec4(0.f),
                    glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
            mRotationalVelocityCubic = glm::mat4x4(0.f);
            return;
        }

        // Scale rotation and rotational velocity such that the rotation is a unit
        // quaternion.
        glm::vec4 x_init = (1.f / rotationLength) * currentRotation;
        const glm::vec4 scaledRotationalVelocity =
                (1.f / rotationLength) * currentRotationalVelocity;

        // Component of 4d velocity that is orthogonal to the current 4d normalized
        // rotation.
        glm::vec4 v_init = scaledRotationalVelocity -
                glm::dot(scaledRotationalVelocity, x_init) * x_init;

        const glm::vec4 x_target = glm::vec4(
                rotation.x, rotation.y, rotation.z, rotation.w);

        if (glm::distance(-x_init, x_target) < glm::distance(x_init, x_target)) {
            // If x_target is closer to the negation of x_init than x_init, then
            // negate x_init.
            x_init = -x_init;
            v_init = -v_init;
        }

        const glm::vec4 cubicTerm = (1.f / kStateChangeTime3) * (
                kStateChangeTime1 * v_init +
                2.f * x_init +
                -2.f * x_target);
        const glm::vec4 quadraticTerm = (1.f / kStateChangeTime2) * (
                -2.f * kStateChangeTime1 * v_init +
                -3.f * x_init +
                3.f * x_target);
        const glm::vec4 linearTerm = v_init;
        const glm::vec4 constantTerm = x_init;

        mRotationCubic = glm::mat4x4(
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);
        mRotationalVelocityCubic = glm::mat4x4(
                glm::vec4(),
                3.f * cubicTerm,
                2.f * quadraticTerm,
                linearTerm);
    }

    mRotationChangeStartTime = mModelTimeNs;
    mRotationChangeEndTime = mModelTimeNs + kStateChangeTimeNs;
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
    const glm::vec4 rotationVec = calculateRotationalState(
            mRotationCubic,
            parameterValueType);

    const glm::quat rotation(
            rotationVec.w, rotationVec.x, rotationVec.y, rotationVec.z);

    return glm::normalize(rotation);
}

glm::vec3 InertialModel::getRotationalVelocity(
        ParameterValueType parameterValueType) const {
    const glm::vec4 rotationVec = calculateRotationalState(
            mRotationCubic,
            parameterValueType);
    const float rotationVecLength = glm::length(rotationVec);

    // Rotation length should not be zero, but it may be possible by driving
    // the inertial model in an extreme way (i.e. well timed oscilations) to
    // hit this case.  In this case, we will simply throw away this target
    // state.
    if (rotationVecLength == 0.f) {
        return glm::vec3(0.f);
    }

    const glm::vec4 rotationNormalized = (1.f / rotationVecLength) * rotationVec;
    const glm::quat rotation = glm::quat(
            rotationNormalized.w,
            rotationNormalized.x,
            rotationNormalized.y,
            rotationNormalized.z);

    const glm::vec4 rotationDerivative = (1.f / rotationVecLength) *
            calculateRotationalState(
                    mRotationalVelocityCubic,
                    parameterValueType);

    const glm::quat rotationDerivativeQuat = glm::quat(
            rotationDerivative.w,
            rotationDerivative.x,
            rotationDerivative.y,
            rotationDerivative.z);

    const glm::quat rotationConjugate = glm::conjugate(rotation);

    const glm::quat angularVelocity =
            2.f * (rotationDerivativeQuat * rotationConjugate);

    return glm::vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z);
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
            endPosition - kStateChangeTime1 * endVelocity);
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

glm::vec4 InertialModel::calculateRotationalState(
        const glm::mat4x4& cubicTransform,
        ParameterValueType parameterValueType) const {
    assert(mModelTimeNs >= mRotationChangeStartTime);
    const uint64_t requestedTimeNs =
            parameterValueType == PARAMETER_VALUE_TYPE_TARGET ?
                    mRotationChangeEndTime :
                    std::min(mModelTimeNs, mRotationChangeEndTime);

    const float time1 = nsToSeconds(requestedTimeNs - mRotationChangeStartTime);
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;
    const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);

    return cubicTransform * cubicTimeVec;
}

}  // namespace physics
}  // namespace android
