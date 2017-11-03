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
    mTestSystem.setUnixTime(1);
    glm::quat newRotation(glm::vec3(180.0f, 0.0f, 0.0f));
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
    glm::quat newRotation(glm::vec3(180.0f, 0.0f, 0.0f));
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

    constexpr uint64_t step = 500UL;
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

    constexpr uint64_t step = 500UL;
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
