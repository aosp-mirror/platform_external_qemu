// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/physics/InertialModel.h"

#include "android/base/testing/GlmTestHelpers.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <glm/gtx/euler_angles.hpp>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;
using android::physics::InertialModel;

constexpr ParameterValueType kValueTypes[] = {
        PARAMETER_VALUE_TYPE_TARGET, PARAMETER_VALUE_TYPE_CURRENT,
        PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION,
        PARAMETER_VALUE_TYPE_DEFAULT};

#define EXPECT_QUAT_NEAR(expected, actual, epsilon) {\
    glm::quat zero = glm::conjugate(expected) * actual;\
    EXPECT_NEAR(0.f, zero.x, epsilon);\
    EXPECT_NEAR(0.f, zero.y, epsilon);\
    EXPECT_NEAR(0.f, zero.z, epsilon);\
    if (zero.w > 0.f) {\
        EXPECT_NEAR(1.f, zero.w, epsilon);\
    } else {\
        EXPECT_NEAR(-1.f, zero.w, epsilon);\
    }\
}

TEST(InertialModel, DefaultPosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    for (auto valueType : kValueTypes) {
        SCOPED_TRACE(testing::Message() << "valueType=" << valueType);

        EXPECT_EQ(inertialModel.getPosition(valueType),
                  glm::vec3(0.f, 0.f, 0.f));
        EXPECT_EQ(inertialModel.getRotation(valueType),
                  glm::quat(1.f, 0.f, 0.f, 0.f));
    }
}

TEST(InertialModel, kDefaultVelocityAndAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    for (auto valueType : kValueTypes) {
        SCOPED_TRACE(testing::Message() << "valueType=" << valueType);

        EXPECT_EQ(inertialModel.getVelocity(valueType),
                  glm::vec3(0.f, 0.f, 0.f));
        EXPECT_EQ(inertialModel.getRotationalVelocity(valueType),
                  glm::vec3(0.f, 0.f, 0.f));
        EXPECT_EQ(inertialModel.getAcceleration(valueType),
                  glm::vec3(0.f, 0.f, 0.f));
    }
}

TEST(InertialModel, DefaultAmbientMotion) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    for (auto valueType : kValueTypes) {
        SCOPED_TRACE(testing::Message() << "valueType=" << valueType);

        EXPECT_EQ(inertialModel.getAmbientMotion(valueType), 0.0f);
    }
}

TEST(InertialModel, ConvergeToPosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);

    glm::vec3 targetPosition(2.0f, 3.0f, 4.0f);
    inertialModel.setTargetPosition(
            targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    inertialModel.setCurrentTime(1000000000UL);

    glm::vec3 currentPosition(inertialModel.getPosition());
    EXPECT_NEAR(targetPosition.x, currentPosition.x, 0.01f);
    EXPECT_NEAR(targetPosition.y, currentPosition.y, 0.01f);
    EXPECT_NEAR(targetPosition.z, currentPosition.z, 0.01f);
}

TEST(InertialModel, AtRestRotationalVelocity) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    InertialModel inertialModel;
    EXPECT_EQ(glm::vec3(0.0f, 0.0f, 0.0f),
                 inertialModel.getRotationalVelocity());
}
TEST(InertialModel, ZeroAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    glm::vec3 targetPosition(2.0f, 3.0f, 4.0f);
    // at 1 second we move the target to (2, 3, 4)
    inertialModel.setTargetPosition(
            targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);
    mTestSystem.setUnixTime(2);
    // at 2 seconds the target is still at (2, 3, 4);
    inertialModel.setTargetPosition(
            targetPosition, PHYSICAL_INTERPOLATION_STEP);
    // the acceleration is expected to be close to zero at this point.
    glm::vec3 currentPosition(inertialModel.getPosition());
    EXPECT_NEAR(targetPosition.x, currentPosition.x, 0.01f);
    EXPECT_NEAR(targetPosition.y, currentPosition.y, 0.01f);
    EXPECT_NEAR(targetPosition.z, currentPosition.z, 0.01f);
}

TEST(InertialModel, ConvergeToRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);

    glm::quat targetRotation(glm::vec3(90.f, 90.f, 90.f));
    inertialModel.setTargetRotation(
            targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    // After 1 second, check that we are close to the target rotation.
    inertialModel.setCurrentTime(1000000000UL);

    glm::quat currentRotation(inertialModel.getRotation());
    EXPECT_QUAT_NEAR(targetRotation, currentRotation, 0.01f);
}

TEST(InertialModel, NonInstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);

    glm::quat startRotation(glm::vec3(0.f, 0.f, 0.f));
    inertialModel.setTargetRotation(
            startRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setCurrentTime(1000000000UL);
    glm::quat newRotation(glm::vec3(-1.0f, 0.0f, 0.0f));
    inertialModel.setTargetRotation(
            newRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    // Verify that the initial rotationalVelocity is zero.
    glm::vec3 initialGyro(inertialModel.getRotationalVelocity());
    EXPECT_NEAR(initialGyro.x, 0.0, 0.000001f);
    EXPECT_NEAR(initialGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(initialGyro.z, 0.0, 0.000001f);

    // Verify that partway through the smooth interpolcaiton, the x component
    // of the linear velocity is non-zero.
    inertialModel.setCurrentTime(1125000000UL);
    glm::vec3 intermediateGyro(inertialModel.getRotationalVelocity());
    EXPECT_LE(intermediateGyro.x, -0.01f);
    EXPECT_NEAR(intermediateGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(intermediateGyro.z, 0.0, 0.000001f);
}

TEST(InertialModel, InstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);

    glm::quat startRotation(glm::vec3(0.f, 0.f, 0.f));
    inertialModel.setTargetRotation(
            startRotation, PHYSICAL_INTERPOLATION_STEP);
    mTestSystem.setUnixTime(1);
    glm::quat newRotation(glm::vec3(-1.0f, 0.0f, 0.0f));
    inertialModel.setTargetRotation(
            newRotation, PHYSICAL_INTERPOLATION_STEP);
    glm::vec3 currentGyro(inertialModel.getRotationalVelocity());
    EXPECT_NEAR(currentGyro.x, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);
}

TEST(InertialModel, SmoothAcceleration) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::vec3 targetPosition (10.0f, 5.0f, 2.0f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(glm::vec3(0.f), PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::vec3 integratedVelocity = glm::vec3(0.f);
    glm::vec3 doubleIntegratedPosition = glm::vec3(0.f);

    glm::vec3 singleIntegratedPosition = glm::vec3(0.f);

    constexpr uint64_t step = 5000UL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 500000000; time += step) {
        inertialModel.setCurrentTime(time);
        integratedVelocity += timeIncrement * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrement * integratedVelocity;

        singleIntegratedPosition += timeIncrement * inertialModel.getVelocity();
    }

    EXPECT_NEAR(doubleIntegratedPosition.x, targetPosition.x, 0.05f);
    EXPECT_NEAR(doubleIntegratedPosition.y, targetPosition.y, 0.05f);
    EXPECT_NEAR(doubleIntegratedPosition.z, targetPosition.z, 0.05f);

    EXPECT_NEAR(singleIntegratedPosition.x, targetPosition.x, 0.05f);
    EXPECT_NEAR(singleIntegratedPosition.y, targetPosition.y, 0.05f);
    EXPECT_NEAR(singleIntegratedPosition.z, targetPosition.z, 0.05f);
}

TEST(InertialModel, MultipleTargets) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::vec3 intermediatePosition (8.0f, 25.0f, 4.0f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(glm::vec3(0.f), PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(intermediatePosition, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::vec3 integratedVelocity = glm::vec3(0.f);
    glm::vec3 doubleIntegratedPosition = glm::vec3(0.f);

    glm::vec3 singleIntegratedPosition = glm::vec3(0.f);

    constexpr uint64_t step = 5000UL;
    constexpr float timeIncrement = step / 1000000000.f;
    uint64_t time = step >> 1;
    for (; time < 250000000; time += step) {
        inertialModel.setCurrentTime(time);
        integratedVelocity += timeIncrement * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrement * integratedVelocity;

        singleIntegratedPosition += timeIncrement * inertialModel.getVelocity();
    }

    glm::vec3 targetPosition (-4.0f, 12.0f, 9.0f);
    inertialModel.setTargetPosition(targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    for (; time < 750000000.f; time += step) {
        inertialModel.setCurrentTime(time);
        integratedVelocity += timeIncrement * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrement * integratedVelocity;

        singleIntegratedPosition += timeIncrement * inertialModel.getVelocity();
    }

    EXPECT_NEAR(doubleIntegratedPosition.x, targetPosition.x, 0.05f);
    EXPECT_NEAR(doubleIntegratedPosition.y, targetPosition.y, 0.05f);
    EXPECT_NEAR(doubleIntegratedPosition.z, targetPosition.z, 0.05f);

    EXPECT_NEAR(singleIntegratedPosition.x, targetPosition.x, 0.05f);
    EXPECT_NEAR(singleIntegratedPosition.y, targetPosition.y, 0.05f);
    EXPECT_NEAR(singleIntegratedPosition.z, targetPosition.z, 0.05f);
}

TEST(InertialModel, TargetPosition) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 targetPosition (14.0f, -3.5f, 0.01f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(glm::vec3(0.f), PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 retrievedTargetPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_TARGET);
    EXPECT_NEAR(targetPosition.x, retrievedTargetPosition.x, 0.0001f);
    EXPECT_NEAR(targetPosition.y, retrievedTargetPosition.y, 0.0001f);
    EXPECT_NEAR(targetPosition.z, retrievedTargetPosition.z, 0.0001f);
}

TEST(InertialModel, TargetVelocity) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 targetVelocity (10.0f, -5.f, 1.f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(glm::vec3(0.f), PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetVelocity(targetVelocity, PHYSICAL_INTERPOLATION_SMOOTH);

    // Retrieve time just before the end to ensure that we hit the case where
    // the pre-interpolation-finished calculation is used.
    inertialModel.setCurrentTime(500000000UL - 1UL);

    const glm::vec3 retrievedStableVelocity =
            inertialModel.getVelocity(PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_NEAR(targetVelocity.x, retrievedStableVelocity.x, 0.0001f);
    EXPECT_NEAR(targetVelocity.y, retrievedStableVelocity.y, 0.0001f);
    EXPECT_NEAR(targetVelocity.z, retrievedStableVelocity.z, 0.0001f);

    const glm::vec3 retrievedStablePosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);

    inertialModel.setCurrentTime(1500000000UL);

    const glm::vec3 oneSecondLaterRetrievedVelocity =
            inertialModel.getVelocity(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(targetVelocity.x, oneSecondLaterRetrievedVelocity.x, 0.0001f);
    EXPECT_NEAR(targetVelocity.y, oneSecondLaterRetrievedVelocity.y, 0.0001f);
    EXPECT_NEAR(targetVelocity.z, oneSecondLaterRetrievedVelocity.z, 0.0001f);

    const glm::vec3 oneSecondLaterRetrievedPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_NEAR(retrievedStablePosition.x + targetVelocity.x,
            oneSecondLaterRetrievedPosition.x, 0.0001f);
    EXPECT_NEAR(retrievedStablePosition.y + targetVelocity.y,
            oneSecondLaterRetrievedPosition.y, 0.0001f);
    EXPECT_NEAR(retrievedStablePosition.z + targetVelocity.z,
            oneSecondLaterRetrievedPosition.z, 0.0001f);
}

TEST(InertialModel, TargetPositionForVelocity) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 velocity(10.0f, -5.f, 1.f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetVelocity(velocity, PHYSICAL_INTERPOLATION_STEP);

    inertialModel.setCurrentTime(500000000UL);

    const glm::vec3 targetPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_TARGET);

    EXPECT_NEAR(targetPosition.x, 0.f, 0.0001f);
    EXPECT_NEAR(targetPosition.y, 0.f, 0.0001f);
    EXPECT_NEAR(targetPosition.z, 0.f, 0.0001f);
}

TEST(InertialModel, TargetVelocityForPosition) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 velocity(3.0f, 8.f, -2.f);
    const glm::vec3 position(10.0f, -5.f, 1.f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetVelocity(velocity, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(position, PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 targetVelocity =
            inertialModel.getVelocity(PARAMETER_VALUE_TYPE_TARGET);

    EXPECT_NEAR(targetVelocity.x, 0.f, 0.0001f);
    EXPECT_NEAR(targetVelocity.y, 0.f, 0.0001f);
    EXPECT_NEAR(targetVelocity.z, 0.f, 0.0001f);
}

TEST(InertialModel, CurrentInitialPositionValue) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 initialPosition (0.1f, -0.4f, 0.6f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(initialPosition, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(glm::vec3(0.3f, 0.9f, -0.5f), PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 retrievedCurrentPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(initialPosition.x, retrievedCurrentPosition.x, 0.0001f);
    EXPECT_NEAR(initialPosition.y, retrievedCurrentPosition.y, 0.0001f);
    EXPECT_NEAR(initialPosition.z, retrievedCurrentPosition.z, 0.0001f);
}


TEST(InertialModel, IntermediateValuesDuringInterpolation) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 initialPosition (0.3f, 0.1f, -2.0f);
    const glm::vec3 targetPosition (5.0f, -2.0f, 1.0f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetPosition(initialPosition, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetPosition(targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::vec3 integratedVelocity = glm::vec3(0.f);
    glm::vec3 doubleIntegratedPosition = initialPosition;

    glm::vec3 singleIntegratedPosition = initialPosition;

    constexpr uint64_t stepNs = 25000UL;
    constexpr float timeIncrementSeconds =
            android::physics::nsToSeconds(stepNs);
    constexpr uint64_t endTimeNs = android::physics::secondsToNs(
            android::physics::kMaxStateChangeTimeSeconds);
    constexpr float epsilon = 0.01f;

    for (uint64_t timeNs = stepNs >> 1; timeNs < endTimeNs; timeNs += stepNs) {
        inertialModel.setCurrentTime(timeNs);
        integratedVelocity +=
                timeIncrementSeconds * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrementSeconds * integratedVelocity;

        const glm::vec3 measuredVelocity = inertialModel.getVelocity();
        EXPECT_NEAR(measuredVelocity.x, integratedVelocity.x, epsilon);
        EXPECT_NEAR(measuredVelocity.y, integratedVelocity.y, epsilon);
        EXPECT_NEAR(measuredVelocity.z, integratedVelocity.z, epsilon);

        singleIntegratedPosition +=
                timeIncrementSeconds *
                inertialModel.getVelocity(PARAMETER_VALUE_TYPE_CURRENT);

        const glm::vec3 measuredPosition =
                inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        EXPECT_NEAR(measuredPosition.x, doubleIntegratedPosition.x, epsilon);
        EXPECT_NEAR(measuredPosition.y, doubleIntegratedPosition.y, epsilon);
        EXPECT_NEAR(measuredPosition.z, doubleIntegratedPosition.z, epsilon);

        EXPECT_NEAR(measuredPosition.x, singleIntegratedPosition.x, epsilon);
        EXPECT_NEAR(measuredPosition.y, singleIntegratedPosition.y, epsilon);
        EXPECT_NEAR(measuredPosition.z, singleIntegratedPosition.z, epsilon);
    }

    // Validate that the velocity and acceleration are zeroed out at the end.
    inertialModel.setCurrentTime(endTimeNs);
    const glm::vec3 measuredAcceleration = inertialModel.getAcceleration();
    EXPECT_NEAR(measuredAcceleration.x, 0.0f, epsilon);
    EXPECT_NEAR(measuredAcceleration.y, 0.0f, epsilon);
    EXPECT_NEAR(measuredAcceleration.z, 0.0f, epsilon);

    const glm::vec3 measuredVelocity = inertialModel.getVelocity();
    EXPECT_NEAR(measuredVelocity.x, 0.0f, epsilon);
    EXPECT_NEAR(measuredVelocity.y, 0.0f, epsilon);
    EXPECT_NEAR(measuredVelocity.z, 0.0f, epsilon);

    const glm::vec3 measuredPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(measuredPosition.x, targetPosition.x, epsilon);
    EXPECT_NEAR(measuredPosition.y, targetPosition.y, epsilon);
    EXPECT_NEAR(measuredPosition.z, targetPosition.z, epsilon);
}

TEST(InertialModel, AmbientMotion) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 initialPosition (0.3f, 0.1f, -2.0f);

    InertialModel inertialModel;
    constexpr uint64_t startTimeNs = (60ULL + 0ULL) * 1000000000ULL;
    constexpr uint64_t midTimeNs = (60ULL + 2ULL) * 1000000000ULL;
    constexpr uint64_t endTimeNs = (60ULL + 4ULL) * 1000000000ULL;
    inertialModel.setCurrentTime(startTimeNs);
    inertialModel.setTargetPosition(initialPosition, PHYSICAL_INTERPOLATION_STEP);

    glm::vec3 integratedVelocity = glm::vec3(0.f);
    glm::vec3 doubleIntegratedPosition = initialPosition;

    glm::vec3 singleIntegratedPosition = initialPosition;

    constexpr uint64_t stepNs = 25000ULL;
    constexpr float timeIncrementSeconds =
            android::physics::nsToSeconds(stepNs);
    constexpr float epsilon = 0.001f;

    bool foundNonZeroPosition = false;
    bool isLatestPositionStable = false;

    inertialModel.setTargetAmbientMotion(0.1f, PHYSICAL_INTERPOLATION_SMOOTH);
    for (uint64_t timeNs = startTimeNs + (stepNs >> 1); timeNs < endTimeNs; timeNs += stepNs) {
        isLatestPositionStable = inertialModel.setCurrentTime(timeNs);
        integratedVelocity +=
                timeIncrementSeconds * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrementSeconds * integratedVelocity;

        const glm::vec3 measuredVelocity = inertialModel.getVelocity();
        EXPECT_NEAR(measuredVelocity.x, integratedVelocity.x, epsilon);
        EXPECT_NEAR(measuredVelocity.y, integratedVelocity.y, epsilon);
        EXPECT_NEAR(measuredVelocity.z, integratedVelocity.z, epsilon);

        singleIntegratedPosition +=
                timeIncrementSeconds *
                inertialModel.getVelocity(PARAMETER_VALUE_TYPE_CURRENT);

        const glm::vec3 measuredPosition =
                inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        EXPECT_NEAR(measuredPosition.x, doubleIntegratedPosition.x, epsilon);
        EXPECT_NEAR(measuredPosition.y, doubleIntegratedPosition.y, epsilon);
        EXPECT_NEAR(measuredPosition.z, doubleIntegratedPosition.z, epsilon);

        EXPECT_NEAR(measuredPosition.x, singleIntegratedPosition.x, epsilon);
        EXPECT_NEAR(measuredPosition.y, singleIntegratedPosition.y, epsilon);
        EXPECT_NEAR(measuredPosition.z, singleIntegratedPosition.z, epsilon);

        if (glm::distance(initialPosition, measuredPosition) >= 0.01f) {
            // we're at least as far away from the initial position as one of
            // the extremes.
            foundNonZeroPosition = true;
        }

        if ((timeNs + stepNs) > midTimeNs && timeNs <= midTimeNs) {
            inertialModel.setTargetAmbientMotion(0.0f, PHYSICAL_INTERPOLATION_SMOOTH);
        }
    }


    EXPECT_TRUE(foundNonZeroPosition);
    EXPECT_TRUE(isLatestPositionStable);

    // Validate that the velocity and acceleration are zeroed out at the end.
    inertialModel.setCurrentTime(endTimeNs);
    const glm::vec3 measuredAcceleration = inertialModel.getAcceleration();
    EXPECT_NEAR(measuredAcceleration.x, 0.0f, epsilon);
    EXPECT_NEAR(measuredAcceleration.y, 0.0f, epsilon);
    EXPECT_NEAR(measuredAcceleration.z, 0.0f, epsilon);

    const glm::vec3 measuredVelocity = inertialModel.getVelocity();
    EXPECT_NEAR(measuredVelocity.x, 0.0f, epsilon);
    EXPECT_NEAR(measuredVelocity.y, 0.0f, epsilon);
    EXPECT_NEAR(measuredVelocity.z, 0.0f, epsilon);

    const glm::vec3 measuredPosition =
            inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(measuredPosition.x, initialPosition.x, epsilon);
    EXPECT_NEAR(measuredPosition.y, initialPosition.y, epsilon);
    EXPECT_NEAR(measuredPosition.z, initialPosition.z, epsilon);
}

TEST(InertialModel, GyroscopeTotalChange) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(90.f),
            glm::radians(90.f),
            glm::radians(0.f)));
    glm::quat targetRotation(glm::eulerAngleXYZ(
            glm::radians(180.f),
            glm::radians(-45.f),
            glm::radians(27.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 5000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 750000000; time += step) {
        inertialModel.setCurrentTime(time);
        const glm::vec3 measuredVelocity =
                inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(
                measuredVelocity.x * timeIncrement,
                measuredVelocity.y * timeIncrement,
                measuredVelocity.z * timeIncrement);

        integratedRotation = glm::quat_cast(rotationMatrix) * integratedRotation;
    }

    EXPECT_QUAT_NEAR(targetRotation, integratedRotation, 0.0001f);
}

TEST(InertialModel, GyroscopeIntermediateValues) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(42.f),
            glm::radians(-34.f),
            glm::radians(110.f)));
    glm::quat targetRotation(glm::eulerAngleXYZ(
            glm::radians(13.f),
            glm::radians(57.f),
            glm::radians(-25.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 10000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 1000000000; time += step) {
        inertialModel.setCurrentTime(time);
        const glm::vec3 measuredVelocity =
                inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(
                measuredVelocity.x * timeIncrement,
                measuredVelocity.y * timeIncrement,
                measuredVelocity.z * timeIncrement);

        integratedRotation =
                glm::quat_cast(rotationMatrix) * integratedRotation;

        glm::quat measuredRotation =
                inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

        EXPECT_QUAT_NEAR(measuredRotation, integratedRotation, 0.0001f);
    }
}


TEST(InertialModel, GyroscopeIntermediateValuesMultiTarget) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(42.f),
            glm::radians(-34.f),
            glm::radians(110.f)));
    glm::quat targetRotation0(glm::eulerAngleXYZ(
            glm::radians(9.f),
            glm::radians(23.f),
            glm::radians(179.f)));
    glm::quat targetRotation1(glm::eulerAngleXYZ(
            glm::radians(-48.f),
            glm::radians(27.f),
            glm::radians(-165.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation0, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 10000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 1000000000; time += step) {
        if (time > 125000000ULL && time - step < 125000000ULL) {
            inertialModel.setTargetRotation(targetRotation1, PHYSICAL_INTERPOLATION_SMOOTH);
        }
        inertialModel.setCurrentTime(time);
        const glm::vec3 measuredVelocity =
                inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(
                measuredVelocity.x * timeIncrement,
                measuredVelocity.y * timeIncrement,
                measuredVelocity.z * timeIncrement);

        integratedRotation =
                glm::quat_cast(rotationMatrix) * integratedRotation;

        glm::quat measuredRotation =
                inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

        EXPECT_QUAT_NEAR(measuredRotation, integratedRotation, 0.0001f);
    }
    EXPECT_QUAT_NEAR(targetRotation1, integratedRotation, 0.0001f);
}

TEST(InertialModel, GyroscopeZeroChange) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(122.f),
            glm::radians(4.f),
            glm::radians(10.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 firstMeasuredVelocity =
            inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.x, 0.0001f);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.y, 0.0001f);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.z, 0.0001f);

    inertialModel.setCurrentTime(1000000000ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 secondMeasuredVelocity =
            inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(0.f, secondMeasuredVelocity.x, 0.0001f);
    EXPECT_NEAR(0.f, secondMeasuredVelocity.y, 0.0001f);
    EXPECT_NEAR(0.f, secondMeasuredVelocity.z, 0.0001f);
}

TEST(InertialModel, Gyroscope180Change) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(0.f),
            glm::radians(0.f),
            glm::radians(0.f)));

    glm::quat targetRotation(glm::eulerAngleXYZ(
            glm::radians(180.f),
            glm::radians(0.f),
            glm::radians(0.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 10000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 1000000000; time += step) {
        inertialModel.setCurrentTime(time);
        const glm::vec3 measuredVelocity =
                inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
        const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(
                measuredVelocity.x * timeIncrement,
                measuredVelocity.y * timeIncrement,
                measuredVelocity.z * timeIncrement);

        integratedRotation =
                glm::quat_cast(rotationMatrix) * integratedRotation;

        glm::quat measuredRotation =
                inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

        EXPECT_QUAT_NEAR(measuredRotation, integratedRotation, 0.0001f);
    }
}

TEST(InertialModel, GyroscopeNaNTest) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(
            0.0000000109016751f,
            0.973329246f,
            -0.0000000339357982f,
            0.229415923f);

    glm::quat targetRotation(
            0.0000000325290905f,
            0.973379254f,
            -0.0000000325290905f,
            0.229200378f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat measuredRotation =
            inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_TRUE(!std::isnan(measuredRotation.x));
    EXPECT_TRUE(!std::isnan(measuredRotation.y));
    EXPECT_TRUE(!std::isnan(measuredRotation.z));
    EXPECT_TRUE(!std::isnan(measuredRotation.w));
}

TEST(InertialModel, GyroscopeUseShortPath) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(0.f),
            glm::radians(-89.f),
            glm::radians(0.f)));

    glm::quat targetRotation(glm::eulerAngleXYZ(
            glm::radians(0.f),
            glm::radians(-91.f),
            glm::radians(0.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation,
            PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation,
            PHYSICAL_INTERPOLATION_SMOOTH);

    inertialModel.setCurrentTime(android::physics::secondsToNs(
            android::physics::kMinStateChangeTimeSeconds / 2.f));

    // Verify that we don't take the long way around even though glm::angle
    // would give us the long way as the default angle between the initial and
    // target rotations.
    glm::vec3 rotationalVelocity = inertialModel.getRotationalVelocity();
    EXPECT_NEAR(0.f, rotationalVelocity.x, 0.00001f);
    EXPECT_GE(-4.f, glm::degrees(rotationalVelocity.y));
    EXPECT_NEAR(0.f, rotationalVelocity.z, 0.00001f);
}

TEST(InertialModel, ManyTargets) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 firstPosition (1.0f, 1.0f, 1.0f);
    const glm::vec3 secondPosition (0.8f, 0.7f, 0.6f);
    const glm::vec3 finalPosition (0.7f, 0.8f, 0.9f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetPosition(firstPosition, PHYSICAL_INTERPOLATION_STEP);

    for (int i = 0; i < 1000; i++) {
        inertialModel.setCurrentTime(i * 50000000ULL);
        inertialModel.setTargetPosition(
                ((i % 1) == 0) ? secondPosition : firstPosition,
                PHYSICAL_INTERPOLATION_SMOOTH);
    }

    glm::vec3 position = inertialModel.getPosition();
    glm::vec3 velocity = inertialModel.getVelocity();

    inertialModel.setCurrentTime(1000ULL * 50000000ULL);
    inertialModel.setTargetPosition(finalPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    constexpr uint64_t step = 5000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    uint64_t time = 50000000ULL * 1000ULL + (step >> 1);
    for (; time < 50000000ULL * 1000ULL + 500000000ULL; time += step) {
        inertialModel.setCurrentTime(time);
        velocity += timeIncrement * inertialModel.getAcceleration();
        position += timeIncrement * velocity;
    }

    const glm::vec3 measuredPosition = inertialModel.getPosition();

    EXPECT_NEAR(measuredPosition.x, position.x, 0.05f);
    EXPECT_NEAR(measuredPosition.y, position.y, 0.05f);
    EXPECT_NEAR(measuredPosition.z, position.z, 0.05f);

    EXPECT_NEAR(finalPosition.x, position.x, 0.05f);
    EXPECT_NEAR(finalPosition.y, position.y, 0.05f);
    EXPECT_NEAR(finalPosition.z, position.z, 0.05f);
}

TEST(InertialModel, ManyTargetVelocities) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::vec3 firstVelocity (1.0f, 1.0f, 1.0f);
    const glm::vec3 secondVelocity (-1.0f, -1.0f, -1.0f);
    const glm::vec3 finalPosition (0.7f, 0.8f, 0.9f);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetPosition(finalPosition, PHYSICAL_INTERPOLATION_STEP);

    for (int i = 0; i < 990; i++) {
        inertialModel.setCurrentTime(i * 50000000ULL);
        inertialModel.setTargetVelocity(
                ((i % 1) == 0) ? secondVelocity : firstVelocity,
                PHYSICAL_INTERPOLATION_SMOOTH);
    }

    inertialModel.setTargetVelocity(glm::vec3(0.0f), PHYSICAL_INTERPOLATION_SMOOTH);
    inertialModel.setCurrentTime(1000ULL * 50000000ULL);

    glm::vec3 position = inertialModel.getPosition();
    glm::vec3 velocity = inertialModel.getVelocity();

    EXPECT_NEAR(0.0f, velocity.x, 0.000001f);
    EXPECT_NEAR(0.0f, velocity.y, 0.000001f);
    EXPECT_NEAR(0.0f, velocity.z, 0.000001f);

    inertialModel.setTargetPosition(finalPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    constexpr uint64_t step = 5000ULL;
    constexpr float timeIncrement = step / 1000000000.f;
    uint64_t time = 50000000ULL * 1000ULL + (step >> 1);
    for (; time < 50000000ULL * 1000ULL + 1000000000ULL; time += step) {
        inertialModel.setCurrentTime(time);
        velocity += timeIncrement * inertialModel.getAcceleration();
        position += timeIncrement * velocity;
    }

    const glm::vec3 measuredPosition = inertialModel.getPosition();

    EXPECT_NEAR(measuredPosition.x, position.x, 0.05f);
    EXPECT_NEAR(measuredPosition.y, position.y, 0.05f);
    EXPECT_NEAR(measuredPosition.z, position.z, 0.05f);

    EXPECT_NEAR(finalPosition.x, position.x, 0.05f);
    EXPECT_NEAR(finalPosition.y, position.y, 0.05f);
    EXPECT_NEAR(finalPosition.z, position.z, 0.05f);

    const glm::vec3 measuredVelocity = inertialModel.getVelocity();

    EXPECT_NEAR(measuredVelocity.x, velocity.x, 0.01f);
    EXPECT_NEAR(measuredVelocity.y, velocity.y, 0.01f);
    EXPECT_NEAR(measuredVelocity.z, velocity.z, 0.01f);

    EXPECT_NEAR(0.0f, measuredVelocity.x, 0.000001f);
    EXPECT_NEAR(0.0f, measuredVelocity.y, 0.000001f);
    EXPECT_NEAR(0.0f, measuredVelocity.z, 0.000001f);
}

TEST(InertialModel, Gyroscope30HzRotationSet) {
    TestSystem testSystem("/", System::kProgramBitness);

    const glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(0.f),
            glm::radians(0.f),
            glm::radians(0.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(0ULL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);

    int rotationXAngleDegrees = 0;
    int rotationYAngleDegrees = 0;
    int rotationZAngleDegrees = 0;
    android::physics::InertialState state =
            android::physics::INERTIAL_STATE_STABLE;
    // time steps in 300ths of a second (so we can do 30 hz target sets and
    // 100hz polling gyroscope).
    int timeSteps = 0;
    glm::quat integratedRotation = initialRotation;
    while (rotationZAngleDegrees > -180 ||
           state != android::physics::INERTIAL_STATE_STABLE) {
        if (timeSteps % 10 == 0 && rotationZAngleDegrees > -180) {
            if (rotationXAngleDegrees < 300) {
                rotationXAngleDegrees++;
            } else if (rotationYAngleDegrees < 180) {
                rotationYAngleDegrees++;
            } else if (rotationZAngleDegrees > -180) {
                rotationZAngleDegrees--;
            }
            const glm::quat targetRotation(glm::eulerAngleXYZ(
                    glm::radians(static_cast<float>(rotationXAngleDegrees)),
                    glm::radians(static_cast<float>(rotationYAngleDegrees)),
                    glm::radians(static_cast<float>(rotationZAngleDegrees))));
            inertialModel.setTargetRotation(
                    targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);
        }
        if (timeSteps % 3 == 0) {
            state = inertialModel.setCurrentTime(timeSteps * 3333333ULL);
            const glm::vec3 measuredVelocity =
                    inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
            const glm::mat4 rotationMatrix = glm::eulerAngleXYZ(
                    measuredVelocity.x * 0.01f,
                    measuredVelocity.y * 0.01f,
                    measuredVelocity.z * 0.01f);

            integratedRotation =
                    glm::quat_cast(rotationMatrix) * integratedRotation;

            const glm::quat measuredRotation =
                    inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

            EXPECT_QUAT_NEAR(measuredRotation, integratedRotation, 0.01f);
        }
        timeSteps++;
    }
    const glm::quat finalRotation(glm::eulerAngleXYZ(
        glm::radians(300.f),
        glm::radians(180.f),
        glm::radians(-180.f)));

    EXPECT_QUAT_NEAR(finalRotation, integratedRotation, 0.01f);

    const glm::quat measuredRotation =
            inertialModel.getRotation(PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_QUAT_NEAR(measuredRotation, integratedRotation, 0.01f);
}
