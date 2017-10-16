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
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    InertialModel inertialModel;
    EXPECT_EQ(glm::vec3(0.f, 0.f, 0.f), inertialModel.getPosition());
    EXPECT_EQ(glm::quat(1.f, 0.f, 0.f, 0.f), inertialModel.getRotation());
}

TEST(InertialModel, DefaultSensorValues) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    InertialModel inertialModel;
    EXPECT_EQ(glm::vec3(0.f, 9.81f, 0.f), inertialModel.getAcceleration());
    EXPECT_EQ(glm::vec3(1.f, 0.f, 0.f), inertialModel.getMagneticVector());
    EXPECT_EQ(glm::vec3(0.f, 0.f, 0.f), inertialModel.getGyroscope());
}

TEST(InertialModel, ConvergeToPosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    InertialModel inertialModel;
    glm::vec3 targetPosition(2.0f, 3.0f, 4.0f);
    inertialModel.setTargetPosition(targetPosition, false);
    // After 1 second, check that we are close to the target position.
    mTestSystem.setUnixTime(1);
    glm::vec3 currentPosition(inertialModel.getPosition());
    EXPECT_NEAR(targetPosition.x, currentPosition.x, 0.01f);
    EXPECT_NEAR(targetPosition.y, currentPosition.y, 0.01f);
    EXPECT_NEAR(targetPosition.z, currentPosition.z, 0.01f);
}

TEST(InertialModel, AtRestGyroscope) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    InertialModel inertialModel;
    EXPECT_EQ(glm::vec3(0.0f, 0.0f, 0.0f),
                 inertialModel.getGyroscope());
}

typedef struct GravityTestCase_ {
    glm::vec3 target_rotation;
    glm::vec3 expected_acceleration;
} GravityTestCase;

const GravityTestCase gravityTestCases[] = {
    {{0.0f, 0.0f, 0.0f}, {0.0f, 9.81f, 0.0f}},
    {{90.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -9.81f}},
    {{-90.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 9.81f}},
    {{0.0f, 90.0f, 0.0f}, {0.0f, 9.81f, 0.0f}},
    {{0.0f, 0.0f, 90.0f}, {9.81f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, -90.0f}, {-9.81f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, 180.0f}, {0.0f, -9.81f, 0.0f}},
};

TEST(InertialModel, GravityAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    for (const auto& testCase : gravityTestCases) {
        mTestSystem.setUnixTime(1);
        InertialModel inertialModel;
        inertialModel.setTargetRotation(
                glm::quat(testCase.target_rotation *
                          (static_cast<float>(M_PI) / 180.0f)), false);
        inertialModel.setTargetPosition(glm::vec3(0.0f, 0.0f, 0.0f), false);
        mTestSystem.setUnixTime(2);
        inertialModel.setTargetPosition(glm::vec3(0.0f, 0.0f, 0.0f), false);
        EXPECT_EQ(glm::vec3(0.f, 0.f, 0.f), inertialModel.getPosition());
        glm::vec3 acceleration(inertialModel.getAcceleration());
        EXPECT_NEAR(testCase.expected_acceleration.x,
                    acceleration.x, 0.01f);
        EXPECT_NEAR(testCase.expected_acceleration.y,
                    acceleration.y, 0.01f);
        EXPECT_NEAR(testCase.expected_acceleration.z,
                    acceleration.z, 0.01f);
    }
}

TEST(InertialModel, ZeroAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    InertialModel inertialModel;
    glm::vec3 targetPosition(2.0f, 3.0f, 4.0f);
    // at 1 second we move the target to (2, 3, 4)
    inertialModel.setTargetPosition(targetPosition, false);
    mTestSystem.setUnixTime(2);
    // at 2 seconds the target is still at (2, 3, 4);
    inertialModel.setTargetPosition(targetPosition, false);
    // the acceleration is expected to be close to zero at this point.
    glm::vec3 currentPosition(inertialModel.getPosition());
    EXPECT_NEAR(targetPosition.x, currentPosition.x, 0.01f);
    EXPECT_NEAR(targetPosition.y, currentPosition.y, 0.01f);
    EXPECT_NEAR(targetPosition.z, currentPosition.z, 0.01f);
}

TEST(InertialModel, ConvergeToRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    InertialModel inertialModel;
    glm::quat targetRotation(glm::vec3(90.f, 90.f, 90.f));
    inertialModel.setTargetRotation(targetRotation, false);
    // After 1 second, check that we are close to the target rotation.
    mTestSystem.setUnixTime(1);
    glm::quat currentRotation(inertialModel.getRotation());
    EXPECT_NEAR(targetRotation.w, currentRotation.w, 0.01f);
    EXPECT_NEAR(targetRotation.x, currentRotation.x, 0.01f);
    EXPECT_NEAR(targetRotation.y, currentRotation.y, 0.01f);
    EXPECT_NEAR(targetRotation.z, currentRotation.z, 0.01f);
}

TEST(InertialModel, NonInstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    InertialModel inertialModel;
    glm::quat startRotation(glm::vec3(0.f, 0.f, 0.f));
    inertialModel.setTargetRotation(startRotation, true);
    mTestSystem.setUnixTime(1);
    glm::quat newRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    inertialModel.setTargetRotation(newRotation, false);
    glm::vec3 currentGyro(inertialModel.getGyroscope());
    EXPECT_LE(currentGyro.x, -0.01f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);
}

TEST(InertialModel, InstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    InertialModel inertialModel;
    glm::quat startRotation(glm::vec3(0.f, 0.f, 0.f));
    inertialModel.setTargetRotation(startRotation, true);
    mTestSystem.setUnixTime(1);
    glm::quat newRotation(glm::vec3(180.0f, 0.0f, 0.0f));
    inertialModel.setTargetRotation(newRotation, true);
    glm::vec3 currentGyro(inertialModel.getGyroscope());
    EXPECT_NEAR(currentGyro.x, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);
}
