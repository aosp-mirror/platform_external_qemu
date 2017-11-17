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

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <glm/gtx/euler_angles.hpp>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;
using android::physics::InertialModel;

TEST(InertialModel, DefaultPosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    EXPECT_EQ(glm::vec3(0.f, 0.f, 0.f), inertialModel.getPosition());
    EXPECT_EQ(glm::quat(1.f, 0.f, 0.f, 0.f), inertialModel.getRotation());
}

TEST(InertialModel, DefaultAccelerationAndRotationalVelocity) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);

    EXPECT_EQ(glm::vec3(0.f, 0.0f, 0.f), inertialModel.getAcceleration());
    EXPECT_EQ(glm::vec3(0.f, 0.f, 0.f), inertialModel.getRotationalVelocity());
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
    EXPECT_NEAR(targetRotation.w, currentRotation.w, 0.01f);
    EXPECT_NEAR(targetRotation.x, currentRotation.x, 0.01f);
    EXPECT_NEAR(targetRotation.y, currentRotation.y, 0.01f);
    EXPECT_NEAR(targetRotation.z, currentRotation.z, 0.01f);
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
    glm::vec3 currentGyro(inertialModel.getRotationalVelocity());
    EXPECT_LE(currentGyro.x, -0.01f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);
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

    inertialModel.setCurrentTime(500000000UL);

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

    constexpr uint64_t step = 50000UL;
    constexpr float timeIncrement = step / 1000000000.f;
    for (uint64_t time = step >> 1; time < 500000000; time += step) {
        inertialModel.setCurrentTime(time);
        integratedVelocity += timeIncrement * inertialModel.getAcceleration();
        doubleIntegratedPosition += timeIncrement * integratedVelocity;

        const glm::vec3 measuredVelocity = inertialModel.getVelocity();
        EXPECT_NEAR(measuredVelocity.x, integratedVelocity.x, 0.01f);
        EXPECT_NEAR(measuredVelocity.y, integratedVelocity.y, 0.01f);
        EXPECT_NEAR(measuredVelocity.z, integratedVelocity.z, 0.01f);

        singleIntegratedPosition += timeIncrement *
                inertialModel.getVelocity(PARAMETER_VALUE_TYPE_CURRENT);

        const glm::vec3 measuredPosition =
                inertialModel.getPosition(PARAMETER_VALUE_TYPE_CURRENT);
        EXPECT_NEAR(measuredPosition.x, doubleIntegratedPosition.x, 0.01f);
        EXPECT_NEAR(measuredPosition.y, doubleIntegratedPosition.y, 0.01f);
        EXPECT_NEAR(measuredPosition.z, doubleIntegratedPosition.z, 0.01f);

        EXPECT_NEAR(measuredPosition.x, singleIntegratedPosition.x, 0.01f);
        EXPECT_NEAR(measuredPosition.y, singleIntegratedPosition.y, 0.01f);
        EXPECT_NEAR(measuredPosition.z, singleIntegratedPosition.z, 0.01f);
    }
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
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 5000UL;
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

    EXPECT_NEAR(targetRotation.x, integratedRotation.x, 0.001f);
    EXPECT_NEAR(targetRotation.y, integratedRotation.y, 0.001f);
    EXPECT_NEAR(targetRotation.z, integratedRotation.z, 0.001f);
    EXPECT_NEAR(targetRotation.w, integratedRotation.w, 0.001f);
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
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 10000UL;
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

        EXPECT_NEAR(measuredRotation.x, integratedRotation.x, 0.01f);
        EXPECT_NEAR(measuredRotation.y, integratedRotation.y, 0.01f);
        EXPECT_NEAR(measuredRotation.z, integratedRotation.z, 0.01f);
        EXPECT_NEAR(measuredRotation.w, integratedRotation.w, 0.01f);
    }
}

TEST(InertialModel, GyroscopeZeroChange) {
    TestSystem testSystem("/", System::kProgramBitness);

    glm::quat initialRotation(glm::eulerAngleXYZ(
            glm::radians(122.f),
            glm::radians(4.f),
            glm::radians(10.f)));

    InertialModel inertialModel;
    inertialModel.setCurrentTime(1000000000UL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    const glm::vec3 firstMeasuredVelocity =
            inertialModel.getRotationalVelocity(PARAMETER_VALUE_TYPE_CURRENT);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.x, 0.0001f);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.y, 0.0001f);
    EXPECT_NEAR(0.f, firstMeasuredVelocity.z, 0.0001f);

    inertialModel.setCurrentTime(1000000000UL);
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
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetRotation(initialRotation, PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    glm::quat integratedRotation = initialRotation;

    constexpr uint64_t step = 10000UL;
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

        EXPECT_NEAR(measuredRotation.x, integratedRotation.x, 0.01f);
        EXPECT_NEAR(measuredRotation.y, integratedRotation.y, 0.01f);
        EXPECT_NEAR(measuredRotation.z, integratedRotation.z, 0.01f);
        EXPECT_NEAR(measuredRotation.w, integratedRotation.w, 0.01f);
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
    inertialModel.setCurrentTime(0UL);
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
    inertialModel.setCurrentTime(0UL);
    inertialModel.setTargetRotation(initialRotation,
            PHYSICAL_INTERPOLATION_STEP);
    inertialModel.setTargetRotation(targetRotation,
            PHYSICAL_INTERPOLATION_SMOOTH);

    // Verify that we don't take the long way around even though glm::angle
    // would give us the long way as the default angle between the initial and
    // target rotations.
    glm::vec3 rotationalVelocity = inertialModel.getRotationalVelocity();
    EXPECT_NEAR(0.f, rotationalVelocity.x, 0.00001f);
    EXPECT_GE(-4.f, glm::degrees(rotationalVelocity.y));
    EXPECT_NEAR(0.f, rotationalVelocity.z, 0.00001f);
}
