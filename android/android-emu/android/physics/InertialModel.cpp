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

#include <glm/gtc/quaternion.hpp>

namespace android {
namespace physics {

constexpr float kEpsilon = 0.0000000001f;

InertialState InertialModel::setCurrentTime(uint64_t time_ns) {
    if (time_ns < mModelTimeNs) {
        // If time goes backwards, set the position and rotation immediately
        // to their targets.
        glm::vec3 targetPosition = getPosition(PARAMETER_VALUE_TYPE_TARGET);
        glm::quat targetRotation = getRotation(PARAMETER_VALUE_TYPE_TARGET);
        mModelTimeNs = time_ns;
        setTargetPosition(targetPosition, PHYSICAL_INTERPOLATION_STEP);
        setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_STEP);
    } else {
        mModelTimeNs = time_ns;
    }

    return (mZeroVelocityAfterEndTime &&
            mModelTimeNs >= mPositionChangeEndTime &&
            mModelTimeNs >= mRotationChangeEndTime &&
            getAmbientMotionBoundsValue(PARAMETER_VALUE_TYPE_CURRENT) <
                    kEpsilon) ?
            INERTIAL_STATE_STABLE : INERTIAL_STATE_CHANGING;
}

void InertialModel::setTargetPosition(
        glm::vec3 position, PhysicalInterpolation mode) {
    float transitionTime = kMinStateChangeTimeSeconds;
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        transitionTime = 0.f;
        const float stateChangeTime1 = transitionTime;
        const float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        const float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        const float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        const float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;
        const float stateChangeTime6 = stateChangeTime3 * stateChangeTime3;
        const float stateChangeTime7 = stateChangeTime3 * stateChangeTime4;

        const glm::vec4 hepticTimeVec = glm::vec4(
                stateChangeTime7, stateChangeTime6, stateChangeTime5, stateChangeTime4);
        const glm::vec4 cubicTimeVec = glm::vec4(
                stateChangeTime3, stateChangeTime2, stateChangeTime1, 1.f);

        // For Step changes, we simply set the transform to immediately take the
        // user to the given position and not reflect any movement-based
        // acceleration or velocity.
        setInertialTransforms(
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                position,
                hepticTimeVec,
                cubicTimeVec);
    } else {
        // We ensure that velocity, acceleration, jerk, and position are
        // continuously interpolating from the current state.  Here, and
        // throughout, x is the position, v is the velocity, a is the
        // acceleration and j is the jerk.
        const glm::vec3 x_init = getPosition(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 v_init = getVelocity(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 a_init = getAcceleration(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 j_init = getJerk(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 x_target = position;

        // Use the square root of distance as the basis for the transition time
        // in order to have a roughly consistent magnitude acceleration.
        const float timeScale = sqrt(glm::distance(x_target, x_init));
        // Use the max time for distances above 10cm.
        const float maxTimeScale = sqrt(0.1f);

        transitionTime = kMinStateChangeTimeSeconds +
                (std::min(timeScale, maxTimeScale) / maxTimeScale) *
                (kMaxStateChangeTimeSeconds - kMinStateChangeTimeSeconds);

        const float stateChangeTime1 = transitionTime;
        const float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        const float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        const float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        const float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;
        const float stateChangeTime6 = stateChangeTime3 * stateChangeTime3;
        const float stateChangeTime7 = stateChangeTime3 * stateChangeTime4;

        const glm::vec4 hepticTimeVec = glm::vec4(
                stateChangeTime7, stateChangeTime6, stateChangeTime5, stateChangeTime4);
        const glm::vec4 cubicTimeVec = glm::vec4(
                stateChangeTime3, stateChangeTime2, stateChangeTime1, 1.f);

        // Computed by solving for heptic movement in
        // stateChangeTimeSeconds. Position, Velocity, Acceleration and Jerk
        // are computed here by solving the system of linear equations created
        // by setting the initial position, velocity, acceleration and jerk to
        // the current values, and the final state to the target position, with
        // no velocity, acceleration or jerk.
        //
        // Equation of motion is:
        //
        // f(t) == At^7 + Bt^6 + Ct^5 + Dt^4 + Et^3 + Ft^2 + Gt + H
        //
        // Where:
        //     A == hepticTerm
        //     B == hexicTerm
        //     C == quinticTerm
        //     D == quarticTerm
        //     E == cubicTerm
        //     F == quadraticTerm
        //     G == linearTerm
        //     H == constantTerm
        // t_end == stateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f(0) == x_init
        //            df/d_t(0) == v_init
        //           d2f/d2t(0) == a_init
        //           d3f/d3t(0) == j_init
        // Final State:
        //             f(t_end) == x_target
        //         df/dt(t_end) == 0
        //       d2f/d2t(t_end) == 0
        //       d3f/d3t(t_end) == 0
        //
        // These can be solved via the following Mathematica command:
        // RowReduce[{{0,0,0,0,0,0,0,1,x},
        //            {0,0,0,0,0,0,1,0,v},
        //            {0,0,0,0,0,2,0,0,a},
        //            {0,0,0,0,6,0,0,0,j},
        //            {t^7,t^6,t^5,t^4,t^3,t^2,t,1,y},
        //            {7t^6,6t^5,5t^4,4t^3,3t^2,2t,1,0,0},
        //            {42t^5,30t^4,20t^3,12t^2,6t,2,0,0,0},
        //            {210t^4,120t^3,60t^2,24t,6,0,0,0,0}}]
        //
        // Where:
        //     x = x_init
        //     v = v_init
        //     a = a_init
        //     j = j_init
        //     t = stateChangeTimeSeconds
        //     y = x_target

        const glm::vec3 hepticTerm = (1.f / (6.f * stateChangeTime7)) * (
                1.f * stateChangeTime3 * j_init +
                12.f * stateChangeTime2 * a_init +
                60.f * stateChangeTime1 * v_init +
                120.f * x_init +
                -120.f * x_target);

        const glm::vec3 hexicTerm = (1.f / (6.f * stateChangeTime6)) * (
                -4.f * stateChangeTime3 * j_init +
                -45.f * stateChangeTime2 * a_init +
                -216.f * stateChangeTime1 * v_init +
                -420.f * x_init +
                420.f * x_target);

        const glm::vec3 quinticTerm = (1.f / (1.f * stateChangeTime5)) * (
                1.f * stateChangeTime3 * j_init +
                10.f * stateChangeTime2 * a_init +
                45.f * stateChangeTime1 * v_init +
                84.f * x_init +
                -84.f * x_target);

        const glm::vec3 quarticTerm = (1.f / (3.f * stateChangeTime4)) * (
                -2.f * stateChangeTime3 * j_init +
                -15.f * stateChangeTime2 * a_init +
                -60.f * stateChangeTime1 * v_init +
                -105.f * x_init +
                105.f * x_target);

        const glm::vec3 cubicTerm = (1.f / 6.f) * j_init;

        const glm::vec3 quadraticTerm = (1.f / 2.f) * a_init;

        const glm::vec3 linearTerm = v_init;

        const glm::vec3 constantTerm = x_init;

        setInertialTransforms(
                hepticTerm,
                hexicTerm,
                quinticTerm,
                quarticTerm,
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm,
                hepticTimeVec,
                cubicTimeVec);
    }
    mPositionChangeStartTime = mModelTimeNs;
    mPositionChangeEndTime = mModelTimeNs + secondsToNs(transitionTime);
    mZeroVelocityAfterEndTime = true;
}

void InertialModel::setTargetVelocity(
        glm::vec3 velocity, PhysicalInterpolation mode) {
    float transitionTime = kMinStateChangeTimeSeconds;
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        transitionTime = 0.f;
        const float stateChangeTime1 = transitionTime;
        const float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        const float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        const float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        const float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;
        const float stateChangeTime6 = stateChangeTime3 * stateChangeTime3;
        const float stateChangeTime7 = stateChangeTime3 * stateChangeTime4;

        const glm::vec4 hepticTimeVec = glm::vec4(
                stateChangeTime7, stateChangeTime6, stateChangeTime5, stateChangeTime4);
        const glm::vec4 cubicTimeVec = glm::vec4(
                stateChangeTime3, stateChangeTime2, stateChangeTime1, 1.f);

        // For Step changes, we simply set the transform to immediately move the
        // user at a given velocity starting from the current position.
        setInertialTransforms(
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                glm::vec3(),
                velocity,
                getPosition(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION),
                hepticTimeVec,
                cubicTimeVec);
    } else {
        // We ensure that velocity, acceleration, jerk, and position are
        // continuously interpolating from the current state.  Here, and
        // throughout, x is the position, v is the velocity, a is the
        // acceleration and j is the jerk.
        const glm::vec3 x_init = getPosition(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 v_init = getVelocity(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 a_init = getAcceleration(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 j_init = getJerk(PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
        const glm::vec3 v_target = velocity;

        // Use the velocity difference as the basis for the transition time
        // in order to have a roughly consistent magnitude acceleration.
        const float timeScale = glm::distance(v_init, v_target);
        // Use the max time for velocity differences above 1m/s.
        constexpr float maxTimeScale = 1.f;

        transitionTime = kMinStateChangeTimeSeconds +
                (std::min(timeScale, maxTimeScale) / maxTimeScale) *
                (kMaxStateChangeTimeSeconds - kMinStateChangeTimeSeconds);

        const float stateChangeTime1 = transitionTime;
        const float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        const float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        const float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        const float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;
        const float stateChangeTime6 = stateChangeTime3 * stateChangeTime3;
        const float stateChangeTime7 = stateChangeTime3 * stateChangeTime4;

        const glm::vec4 hepticTimeVec = glm::vec4(
                stateChangeTime7, stateChangeTime6, stateChangeTime5, stateChangeTime4);
        const glm::vec4 cubicTimeVec = glm::vec4(
                stateChangeTime3, stateChangeTime2, stateChangeTime1, 1.f);

        // Computed by solving for hexic movement in
        // stateChangeTimeSeconds. Position, Velocity, Acceleration, and Jerk
        // are computed here by solving the system of linear equations created
        // by setting the initial position, velocity, acceleration and jerk to
        // the current values, and the final state to the target velocity, with
        // no acceleration or jerk.  Note that there is no target position
        // specified.
        //
        // Equation of motion is:
        //
        // f(t) == At^6 + Bt^5 + Ct^4 + Dt^3 + Et^2 + Ft + G
        //
        // Where:
        //     A == hexicTerm
        //     B == quinticTerm
        //     C == quarticTerm
        //     D == cubicTerm
        //     E == quadraticTerm
        //     F == linearTerm
        //     G == constantTerm
        // t_end == stateChangeTimeSeconds
        //
        // And this system of equations is solved:
        //
        // Initial State:
        //                 f(0) == x_init
        //            df/d_t(0) == v_init
        //           d2f/d2t(0) == a_init
        //           d3f/d3t(0) == j_init
        // Final State:
        //         df/dt(t_end) == v_target
        //       d2f/d2t(t_end) == 0
        //       d3f/d3t(t_end) == 0
        //
        // These can be solved via the following Mathematica command:
        // RowReduce[{{0,0,0,0,0,0,1,x},
        //            {0,0,0,0,0,1,0,v},
        //            {0,0,0,0,2,0,0,a},
        //            {0,0,0,6,0,0,0,j},
        //            {6t^5,5t^4,4t^3,3t^2,2t,1,0,w},
        //            {30t^4,20t^3,12t^2,6t,2,0,0,0},
        //            {120t^3,60t^2,24t,6,0,0,0,0}}]
        //
        // Where:
        //     x = x_init
        //     v = v_init
        //     a = a_init
        //     j = j_init
        //     t = stateChangeTimeSeconds
        //     w = v_target

        const glm::vec3 hexicTerm = (1.f / (12.f * stateChangeTime5)) * (
                -1.f * stateChangeTime2 * j_init +
                -6.f * stateChangeTime1 * a_init +
                -12.f * v_init +
                12.f * v_target);
        const glm::vec3 quinticTerm = (1.f / (10.f * stateChangeTime4)) * (
                3.f * stateChangeTime2 * j_init +
                16.f * stateChangeTime1 * a_init +
                30.f * v_init +
                -30.f * v_target);
        const glm::vec3 quarticTerm = (1.f / (8.f * stateChangeTime3)) * (
                -3.f * stateChangeTime2 * j_init +
                -12.f * stateChangeTime1 * a_init +
                -20.f * v_init +
                20.f * v_target);
        const glm::vec3 cubicTerm = (1.f / 6.f) * j_init;
        const glm::vec3 quadraticTerm = (1.f / 2.f) * a_init;
        const glm::vec3 linearTerm = v_init;
        const glm::vec3 constantTerm = x_init;

        setInertialTransforms(
                glm::vec3(0.0f),
                hexicTerm,
                quinticTerm,
                quarticTerm,
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm,
                hepticTimeVec,
                cubicTimeVec);
    }
    mPositionChangeStartTime = mModelTimeNs;
    mPositionChangeEndTime = mModelTimeNs + secondsToNs(transitionTime);
    mZeroVelocityAfterEndTime = glm::length(velocity) <= kEpsilon;
}

void InertialModel::setTargetRotation(
        glm::quat rotation, PhysicalInterpolation mode) {
    float transitionTime = kMinStateChangeTimeSeconds;
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        transitionTime = 0.f;
        // For Step changes, we simply set the transform to immediately set the
        // rotation to the target, with zero rotational velocity.
        mRotationQuintic = glm::mat2x4(
                glm::vec4(0.f),
                glm::vec4(0.f));
        mRotationCubic = glm::mat4x4(
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
        mRotationAfterEndCubic = mRotationCubic;
        mRotationalVelocityQuintic = glm::mat2x4(0.f);
        mRotationalVelocityCubic = glm::mat4x4(0.f);
        mRotationalAccelerationQuintic = glm::mat2x4(0.f);
        mRotationalAccelerationCubic = glm::mat4x4(0.f);
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
        //            df/d_t(0) == a_init
        // Final State:
        //             f(t_end) == x_target
        //         df/dt(t_end) == 0
        //        df/d_t(t_end) == 0
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
        //     t = stateChangeTimeSeconds
        //     y = x_target

        const glm::vec4 currentRotation = calculateRotationalState(
                mRotationQuintic, mRotationCubic, mRotationAfterEndCubic,
                PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);

        const glm::vec4 currentRotationalVelocity = calculateRotationalState(
                mRotationalVelocityQuintic, mRotationalVelocityCubic,
                glm::mat4x4(0.f), PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);

        const glm::vec4 currentRotationalAcceleration = calculateRotationalState(
                mRotationalAccelerationQuintic, mRotationalAccelerationCubic,
                glm::mat4x4(0.f), PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);

        const float rotationLength = glm::length(currentRotation);

        // Rotation length should not be zero, but it may be possible by driving
        // the inertial model in an extreme way (i.e. well timed oscilations) to
        // hit this case.  In this case, we will simply do a step to the target.
        if (rotationLength == 0.f) {
            mRotationQuintic = glm::mat2x4(
                    glm::vec4(0.f),
                    glm::vec4(0.f));
            mRotationCubic = glm::mat4x4(
                    glm::vec4(0.f),
                    glm::vec4(0.f),
                    glm::vec4(0.f),
                    glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
            mRotationAfterEndCubic = mRotationCubic;
            mRotationalVelocityQuintic = glm::mat2x4(0.f);
            mRotationalVelocityCubic = glm::mat4x4(0.f);
            mRotationalAccelerationQuintic = glm::mat2x4(0.f);
            mRotationalAccelerationCubic = glm::mat4x4(0.f);
            return;
        }

        // Scale rotation and rotational velocity such that the rotation is a unit
        // quaternion.
        glm::vec4 x_init = (1.f / rotationLength) * currentRotation;

        // Component of 4d velocity that is orthogonal to the current 4d normalized
        // rotation.
        const glm::vec4 scaledRotationalVelocity =
                (1.f / rotationLength) * currentRotationalVelocity;
        glm::vec4 v_init = scaledRotationalVelocity -
                glm::dot(scaledRotationalVelocity, x_init) * x_init;

        // Component of 4d acceleration that is orthogonal to the current 4d normalized
        // rotation.
        const glm::vec4 scaledRotationalAcceleration =
                (1.f / rotationLength) * currentRotationalAcceleration;
        glm::vec4 a_init = scaledRotationalAcceleration -
                glm::dot(scaledRotationalAcceleration, x_init) * x_init;

        const glm::vec4 x_target = glm::vec4(
                rotation.x, rotation.y, rotation.z, rotation.w);

        if (glm::distance(-x_init, x_target) < glm::distance(x_init, x_target)) {
            // If x_target is closer to the negation of x_init than x_init, then
            // negate x_init.
            x_init = -x_init;
            v_init = -v_init;
        }

        // Use the square root of the rotation distance as the basis for the
        // transition time in order to have a roughly consistent magnitude of
        // angular acceleration.
        const float timeScale = sqrt(glm::distance(x_init, x_target));
        // Use the max time for transitions of 180 degrees (i.e. distance 2 in
        // quaternion space).
        const float maxTimeScale = sqrt(2.f);

        transitionTime = kMinStateChangeTimeSeconds +
                (std::min(timeScale, maxTimeScale) / maxTimeScale) *
                (kMaxStateChangeTimeSeconds - kMinStateChangeTimeSeconds);

        const float stateChangeTime1 = transitionTime;
        const float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        const float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        const float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        const float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;

        const glm::vec4 quinticTerm = (1.f / (2.0f * stateChangeTime5)) * (
                -1.f * stateChangeTime2 * a_init +
                -6.f * stateChangeTime1 * v_init +
                -12.f * x_init +
                12.f * x_target);
        const glm::vec4 quarticTerm = (1.f / (2.0f * stateChangeTime4)) * (
                3.f * stateChangeTime2 * a_init +
                16.f * stateChangeTime1 * v_init +
                30.f * x_init +
                -30.f * x_target);
        const glm::vec4 cubicTerm = (1.f / (2.0f * stateChangeTime3)) * (
                -3.f * stateChangeTime2 * a_init +
                -12.f * stateChangeTime1 * v_init +
                -20.f * x_init +
                20.f * x_target);
        const glm::vec4 quadraticTerm = (1.f / 2.f) * a_init;
        const glm::vec4 linearTerm = v_init;
        const glm::vec4 constantTerm = x_init;

        mRotationQuintic = glm::mat2x4(
                quinticTerm,
                quarticTerm);
        mRotationCubic = glm::mat4x4(
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);
        mRotationAfterEndCubic = glm::mat4x4(
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(0.f),
                glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
        mRotationalVelocityQuintic = glm::mat2x4(
                glm::vec4(),
                5.f * quinticTerm);
        mRotationalVelocityCubic = glm::mat4x4(
                4.f * quarticTerm,
                3.f * cubicTerm,
                2.f * quadraticTerm,
                linearTerm);
        mRotationalAccelerationQuintic = glm::mat2x4(
                glm::vec4(),
                glm::vec4());
        mRotationalAccelerationCubic = glm::mat4x4(
                20.f * quinticTerm,
                12.f * quarticTerm,
                6.f * cubicTerm,
                2.f * quadraticTerm);
    }

    mRotationChangeStartTime = mModelTimeNs;
    mRotationChangeEndTime = mModelTimeNs + secondsToNs(transitionTime);
}

void InertialModel::setTargetAmbientMotion(float bounds,
                                           PhysicalInterpolation mode) {
    if (mode == PHYSICAL_INTERPOLATION_STEP) {
        mAmbientMotionValueQuintic = glm::vec2(0.f);
        mAmbientMotionValueCubic = glm::vec4(
                0.f,
                0.f,
                0.f,
                bounds);
        mAmbientMotionFirstDerivQuintic = glm::vec2(0.f);
        mAmbientMotionFirstDerivCubic = glm::vec4(0.f);
        mAmbientMotionSecondDerivQuintic = glm::vec2(0.f);
        mAmbientMotionSecondDerivCubic = glm::vec4(0.f);
        mAmbientMotionChangeStartTime = mModelTimeNs;
        mAmbientMotionChangeEndTime = mModelTimeNs;
    } else {
        // Ambient motion bounds expansion needs to be differentiable so we can
        // always compute the acceleration of the ambient motion.  This does the
        // same polynomial computation as the quintic rotation above with the
        // same coefficients, but in one dimension instead of 4.

        float x_init = getAmbientMotionBoundsValue(PARAMETER_VALUE_TYPE_CURRENT);
        float v_init = getAmbientMotionBoundsDeriv(PARAMETER_VALUE_TYPE_CURRENT);
        float a_init = getAmbientMotionBoundsSecondDeriv(PARAMETER_VALUE_TYPE_CURRENT);
        float x_target = bounds;

        constexpr float stateChangeTime1 = kMaxStateChangeTimeSeconds;
        constexpr float stateChangeTime2 = stateChangeTime1 * stateChangeTime1;
        constexpr float stateChangeTime3 = stateChangeTime1 * stateChangeTime2;
        constexpr float stateChangeTime4 = stateChangeTime2 * stateChangeTime2;
        constexpr float stateChangeTime5 = stateChangeTime2 * stateChangeTime3;

        const float quinticTerm = (1.f / (2.0f * stateChangeTime5)) * (
                -1.f * stateChangeTime2 * a_init +
                -6.f * stateChangeTime1 * v_init +
                -12.f * x_init +
                12.f * x_target);
        const float quarticTerm = (1.f / (2.0f * stateChangeTime4)) * (
                3.f * stateChangeTime2 * a_init +
                16.f * stateChangeTime1 * v_init +
                30.f * x_init +
                -30.f * x_target);
        const float cubicTerm = (1.f / (2.0f * stateChangeTime3)) * (
                -3.f * stateChangeTime2 * a_init +
                -12.f * stateChangeTime1 * v_init +
                -20.f * x_init +
                20.f * x_target);
        const float quadraticTerm = (1.f / 2.f) * a_init;
        const float linearTerm = v_init;
        const float constantTerm = x_init;

        mAmbientMotionEndValue = bounds;
        mAmbientMotionValueQuintic = glm::vec2(
                quinticTerm,
                quarticTerm);
        mAmbientMotionValueCubic = glm::vec4(
                cubicTerm,
                quadraticTerm,
                linearTerm,
                constantTerm);
        mAmbientMotionFirstDerivQuintic = glm::vec2(
                0.f,
                5.f * quinticTerm);
        mAmbientMotionFirstDerivCubic = glm::vec4(
                4.f * quarticTerm,
                3.f * cubicTerm,
                2.f * quadraticTerm,
                linearTerm);
        mAmbientMotionSecondDerivQuintic = glm::vec2(
                0.f,
                0.f);
        mAmbientMotionSecondDerivCubic = glm::vec4(
                20.f * quinticTerm,
                12.f * quarticTerm,
                6.f * cubicTerm,
                2.f * quadraticTerm);

        mAmbientMotionChangeStartTime = mModelTimeNs;
        mAmbientMotionChangeEndTime = mModelTimeNs +
                secondsToNs(stateChangeTime1);
    }
}

glm::vec3 InertialModel::getPosition(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::vec3();
    }

    glm::vec3 position = calculateInertialState(
            mPositionHeptic,
            mPositionCubic,
            mPositionAfterEndCubic,
            parameterValueType);
    if (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT) {
        double time = mModelTimeNs / 1000000000.0;
        glm::vec3 f = glm::vec3(sin(kAmbientFrequencyVec.x * time),
                sin(kAmbientFrequencyVec.y * time),
                sin(kAmbientFrequencyVec.z * time));
        glm::vec3 g = glm::vec3(1.f) * getAmbientMotionBoundsValue(PARAMETER_VALUE_TYPE_CURRENT);
        position += f * g;
    }
    return position;
}

glm::vec3 InertialModel::getVelocity(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::vec3();
    }

    glm::vec3 velocity = calculateInertialState(
        mVelocityHeptic,
        mVelocityCubic,
        mZeroVelocityAfterEndTime ? glm::mat4x3(0.f) : mVelocityAfterEndCubic,
        parameterValueType);
    if (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT) {
        double time = mModelTimeNs / 1000000000.0;
        glm::vec3 f = glm::vec3(sin(kAmbientFrequencyVec.x * time),
                sin(kAmbientFrequencyVec.y * time),
                sin(kAmbientFrequencyVec.z * time));
        // Apply the chain rule.
        glm::vec3 df = glm::vec3(cos(kAmbientFrequencyVec.x * time),
                cos(kAmbientFrequencyVec.y * time),
                cos(kAmbientFrequencyVec.z * time)) * kAmbientFrequencyVec;
        glm::vec3 g = glm::vec3(1.f) * getAmbientMotionBoundsValue(PARAMETER_VALUE_TYPE_CURRENT);
        glm::vec3 dg = glm::vec3(1.f) * getAmbientMotionBoundsDeriv(PARAMETER_VALUE_TYPE_CURRENT);
        // Apply the product rule.
        velocity += f * dg + df * g;
    }
    return velocity;
}

glm::vec3 InertialModel::getAcceleration(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::vec3();
    }

    glm::vec3 acceleration = calculateInertialState(
        mAccelerationHeptic,
        mAccelerationCubic,
        glm::mat4x3(0.f),
        parameterValueType);
    if (parameterValueType == PARAMETER_VALUE_TYPE_CURRENT) {
        double time = mModelTimeNs / 1000000000.0;
        glm::vec3 f = glm::vec3(sin(kAmbientFrequencyVec.x * time),
                sin(kAmbientFrequencyVec.y * time),
                sin(kAmbientFrequencyVec.z * time));
        // Apply the chain rule.
        glm::vec3 df = glm::vec3(cos(kAmbientFrequencyVec.x * time),
                cos(kAmbientFrequencyVec.y * time),
                cos(kAmbientFrequencyVec.z * time)) * kAmbientFrequencyVec;
        // Apply the chain rule twice.
        glm::vec3 d2f = glm::vec3(-sinf(kAmbientFrequencyVec.x * time),
                -sin(kAmbientFrequencyVec.y * time),
                -sin(kAmbientFrequencyVec.z * time)) * kAmbientFrequencyVec * kAmbientFrequencyVec;
        glm::vec3 g = glm::vec3(1.f) * getAmbientMotionBoundsValue(PARAMETER_VALUE_TYPE_CURRENT);
        glm::vec3 dg = glm::vec3(1.f) * getAmbientMotionBoundsDeriv(PARAMETER_VALUE_TYPE_CURRENT);
        glm::vec3 d2g = glm::vec3(1.f) * getAmbientMotionBoundsSecondDeriv(PARAMETER_VALUE_TYPE_CURRENT);
        // Apply the product rule twice.
        acceleration += f * d2g + 2.f * df * dg + d2f * g;
    }
    return acceleration;
}

glm::vec3 InertialModel::getJerk(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::vec3();
    }

    return calculateInertialState(
        mJerkHeptic,
        mJerkCubic,
        glm::mat4x3(0.f),
        parameterValueType);
}

glm::quat InertialModel::getRotation(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::quat();
    }

    const glm::vec4 rotationVec = calculateRotationalState(
            mRotationQuintic,
            mRotationCubic,
            mRotationAfterEndCubic,
            parameterValueType);

    const glm::quat rotation(
            rotationVec.w, rotationVec.x, rotationVec.y, rotationVec.z);

    return glm::normalize(rotation);
}

glm::vec3 InertialModel::getRotationalVelocity(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return glm::vec3();
    }

    const glm::vec4 rotationVec = calculateRotationalState(
            mRotationQuintic,
            mRotationCubic,
            mRotationAfterEndCubic,
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

    const glm::vec4 scaledDerivative = (1.f / rotationVecLength) *
            calculateRotationalState(
                    mRotationalVelocityQuintic,
                    mRotationalVelocityCubic,
                    glm::mat4x4(0.f),
                    parameterValueType);

    const glm::vec4 rotationDerivative = scaledDerivative -
            glm::dot(scaledDerivative, rotationNormalized) * rotationNormalized;

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

float InertialModel::getAmbientMotion(
        ParameterValueType parameterValueType) const {
    return getAmbientMotionBoundsValue(parameterValueType);
}

void InertialModel::setInertialTransforms(
        const glm::vec3& hepticCoefficient,
        const glm::vec3& hexicCoefficient,
        const glm::vec3& quinticCoefficient,
        const glm::vec3& quarticCoefficient,
        const glm::vec3& cubicCoefficient,
        const glm::vec3& quadraticCoefficient,
        const glm::vec3& linearCoefficient,
        const glm::vec3& constantCoefficient,
        const glm::vec4& hepticTimeVec,
        const glm::vec4& cubicTimeVec) {
    mPositionHeptic = glm::mat4x3(
            hepticCoefficient,
            hexicCoefficient,
            quinticCoefficient,
            quarticCoefficient);
    mPositionCubic = glm::mat4x3(
            cubicCoefficient,
            quadraticCoefficient,
            linearCoefficient,
            constantCoefficient);

    mVelocityHeptic = glm::mat4x3(
            glm::vec3(0.0f),
            7.f * hepticCoefficient,
            6.f * hexicCoefficient,
            5.f * quinticCoefficient);
    mVelocityCubic = glm::mat4x3(
            4.f * quarticCoefficient,
            3.f * cubicCoefficient,
            2.f * quadraticCoefficient,
            linearCoefficient);

    mAccelerationHeptic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            42.f * hepticCoefficient,
            30.f * hexicCoefficient);
    mAccelerationCubic = glm::mat4x3(
            20.f * quinticCoefficient,
            12.f * quarticCoefficient,
            6.f * cubicCoefficient,
            2.f * quadraticCoefficient);

    mJerkHeptic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            210.f * hepticCoefficient);
    mJerkCubic = glm::mat4x3(
            120.f * hexicCoefficient,
            60.f * quinticCoefficient,
            24.f * quarticCoefficient,
            6.f * cubicCoefficient);

    glm::vec3 endPosition = mPositionCubic * cubicTimeVec +
            mPositionHeptic * hepticTimeVec;
    glm::vec3 endVelocity = mVelocityCubic * cubicTimeVec +
            mVelocityHeptic * hepticTimeVec;

    mPositionAfterEndCubic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            endVelocity,
            endPosition - cubicTimeVec.z * endVelocity);
    mVelocityAfterEndCubic = glm::mat4x3(
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            endVelocity);
}

glm::vec3 InertialModel::calculateInertialState(
        const glm::mat4x3& hepticTransform,
        const glm::mat4x3& cubicTransform,
        const glm::mat4x3& afterEndCubicTransform,
        ParameterValueType parameterValueType) const {
    const uint64_t requestedTimeNs =
            parameterValueType == PARAMETER_VALUE_TYPE_TARGET ?
                    mPositionChangeEndTime : mModelTimeNs;

    const float time1 = nsToSeconds(requestedTimeNs - mPositionChangeStartTime);
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;
    const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);

    if (requestedTimeNs < mPositionChangeEndTime) {
        const float time4 = time2 * time2;
        const float time5 = time2 * time3;
        const float time6 = time3 * time3;
        const float time7 = time3 * time4;
        const glm::vec4 hepticTimeVec(time7, time6, time5, time4);
        return cubicTransform * cubicTimeVec + hepticTransform * hepticTimeVec;
    } else {
        return afterEndCubicTransform * cubicTimeVec;
    }
}

glm::vec4 InertialModel::calculateRotationalState(
        const glm::mat2x4& quinticTransform,
        const glm::mat4x4& cubicTransform,
        const glm::mat4x4& afterEndCubicTransform,
        ParameterValueType parameterValueType) const {
    const uint64_t requestedTimeNs =
            parameterValueType == PARAMETER_VALUE_TYPE_TARGET ?
                    mRotationChangeEndTime : mModelTimeNs;

    const float time1 = nsToSeconds(requestedTimeNs - mRotationChangeStartTime);
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;
    const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);
    if (requestedTimeNs < mRotationChangeEndTime) {
        const float time4 = time2 * time2;
        const float time5 = time3 * time2;
        const glm::vec2 quinticTimeVec(time5, time4);
        return quinticTransform * quinticTimeVec + cubicTransform * cubicTimeVec;
    } else {
        return afterEndCubicTransform * cubicTimeVec;
    }
}

float InertialModel::getAmbientMotionBoundsValue(
        ParameterValueType parameterValueType) const {
    if (parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT) {
        return 0.f;
    } else if (parameterValueType != PARAMETER_VALUE_TYPE_TARGET &&
               mModelTimeNs < mAmbientMotionChangeEndTime) {
        const float time1 = nsToSeconds(mModelTimeNs - mAmbientMotionChangeStartTime);
        const float time2 = time1 * time1;
        const float time3 = time2 * time1;
        const float time4 = time2 * time2;
        const float time5 = time3 * time2;
        const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);
        const glm::vec2 quinticTimeVec(time5, time4);
        return glm::dot(mAmbientMotionValueQuintic, quinticTimeVec) +
                glm::dot(mAmbientMotionValueCubic, cubicTimeVec);
    } else {
        return mAmbientMotionEndValue;
    }
}

float InertialModel::getAmbientMotionBoundsDeriv(
        ParameterValueType parameterValueType) const {
    if (parameterValueType != PARAMETER_VALUE_TYPE_TARGET &&
            mModelTimeNs < mAmbientMotionChangeEndTime) {
        const float time1 = nsToSeconds(mModelTimeNs - mAmbientMotionChangeStartTime);
        const float time2 = time1 * time1;
        const float time3 = time2 * time1;
        const float time4 = time2 * time2;
        const float time5 = time3 * time2;
        const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);
        const glm::vec2 quinticTimeVec(time5, time4);
        return glm::dot(mAmbientMotionFirstDerivQuintic, quinticTimeVec) +
                glm::dot(mAmbientMotionFirstDerivCubic, cubicTimeVec);
    } else {
        return 0.f;
    }
}

float InertialModel::getAmbientMotionBoundsSecondDeriv(
        ParameterValueType parameterValueType) const {
    if (parameterValueType != PARAMETER_VALUE_TYPE_TARGET &&
            mModelTimeNs < mAmbientMotionChangeEndTime) {
        const float time1 = nsToSeconds(mModelTimeNs - mAmbientMotionChangeStartTime);
        const float time2 = time1 * time1;
        const float time3 = time2 * time1;
        const float time4 = time2 * time2;
        const float time5 = time3 * time2;
        const glm::vec4 cubicTimeVec(time3, time2, time1, 1.f);
        const glm::vec2 quinticTimeVec(time5, time4);
        return glm::dot(mAmbientMotionSecondDerivQuintic, quinticTimeVec) +
                glm::dot(mAmbientMotionSecondDerivCubic, cubicTimeVec);
    } else {
        return 0.f;
    }
}

}  // namespace physics
}  // namespace android
