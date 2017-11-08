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

#include "android/physics/PhysicalModel.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;

TEST(PhysicalModel, CreateAndDestroy) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new();
    EXPECT_NE(model, nullptr);
    physicalModel_free(model);
}

TEST(PhysicalModel, DefaultInertialSensorValues) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    PhysicalModel *model = physicalModel_new();
    long measurement_id;
    vec3 accelerometer = physicalModel_getAccelerometer(model,
            &measurement_id);
    EXPECT_NEAR(0.f, accelerometer.x, 0.001f);
    EXPECT_NEAR(9.81f, accelerometer.y, 0.001f);
    EXPECT_NEAR(0.f, accelerometer.z, 0.001f);

    vec3 gyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_NEAR(0.f, gyro.x, 0.001f);
    EXPECT_NEAR(0.f, gyro.y, 0.001f);
    EXPECT_NEAR(0.f, gyro.z, 0.001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, ConstantMeasurementId) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    PhysicalModel *model = physicalModel_new();
    long measurement_id0;
    physicalModel_getAccelerometer(model, &measurement_id0);

    mTestSystem.setUnixTime(2);

    long measurement_id1;
    physicalModel_getAccelerometer(model, &measurement_id1);

    EXPECT_EQ(measurement_id0, measurement_id1);

    physicalModel_free(model);
}


TEST(PhysicalModel, NewMeasurementId) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    PhysicalModel *model = physicalModel_new();
    long measurement_id0;
    physicalModel_getAccelerometer(model, &measurement_id0);

    mTestSystem.setUnixTime(2);

    vec3 targetPosition;
    targetPosition.x = 2.0f;
    targetPosition.y = 3.0f;
    targetPosition.z = 4.0f;
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    long measurement_id1;
    physicalModel_getAccelerometer(model, &measurement_id1);

    EXPECT_NE(measurement_id0, measurement_id1);

    physicalModel_free(model);
}


TEST(PhysicalModel, SetTargetPosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    PhysicalModel *model = physicalModel_new();
    vec3 targetPosition;
    targetPosition.x = 2.0f;
    targetPosition.y = 3.0f;
    targetPosition.z = 4.0f;
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_STEP);

    mTestSystem.setUnixTime(1);
    vec3 currentPosition = physicalModel_getTargetPosition(model);

    EXPECT_NEAR(targetPosition.x, currentPosition.x, 0.0001f);
    EXPECT_NEAR(targetPosition.y, currentPosition.y, 0.0001f);
    EXPECT_NEAR(targetPosition.z, currentPosition.z, 0.0001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, SetTargetRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    PhysicalModel *model = physicalModel_new();
    vec3 targetRotation;
    targetRotation.x = 45.0f;
    targetRotation.y = 10.0f;
    targetRotation.z = 4.0f;
    physicalModel_setTargetRotation(
            model, targetRotation, PHYSICAL_INTERPOLATION_STEP);

    mTestSystem.setUnixTime(1);
    vec3 currentRotation = physicalModel_getTargetRotation(model);

    EXPECT_NEAR(targetRotation.x, currentRotation.x, 0.0001f);
    EXPECT_NEAR(targetRotation.y, currentRotation.y, 0.0001f);
    EXPECT_NEAR(targetRotation.z, currentRotation.z, 0.0001f);

    physicalModel_free(model);
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

TEST(PhysicalModel, GravityAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    for (const auto& testCase : gravityTestCases) {
        mTestSystem.setUnixTime(1);
        PhysicalModel* model = physicalModel_new();

        vec3 targetRotation;
        targetRotation.x = testCase.target_rotation.x;
        targetRotation.y = testCase.target_rotation.y;
        targetRotation.z = testCase.target_rotation.z;

        physicalModel_setTargetRotation(
                model, targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

        mTestSystem.setUnixTime(2);

        long measurement_id;
        vec3 accelerometer = physicalModel_getAccelerometer(model,
                &measurement_id);

        EXPECT_NEAR(testCase.expected_acceleration.x,
                    accelerometer.x, 0.01f);
        EXPECT_NEAR(testCase.expected_acceleration.y,
                    accelerometer.y, 0.01f);
        EXPECT_NEAR(testCase.expected_acceleration.z,
                    accelerometer.z, 0.01f);

        physicalModel_free(model);
    }
}

TEST(PhysicalModel, GravityOnlyAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);

    PhysicalModel* model = physicalModel_new();

    vec3 targetPosition;
    targetPosition.x = 2.0f;
    targetPosition.y = 3.0f;
    targetPosition.z = 4.0f;
    // at 1 second we move the target to (2, 3, 4)
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    mTestSystem.setUnixTime(2);
    // at 2 seconds the target is still at (2, 3, 4);
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_STEP);

    long measurement_id;
    // the acceleration is expected to be close to zero at this point.
    vec3 currentAcceleration = physicalModel_getAccelerometer(model,
            &measurement_id);
    EXPECT_NEAR(currentAcceleration.x, 0.f, 0.01f);
    EXPECT_NEAR(currentAcceleration.y, 9.81f, 0.01f);
    EXPECT_NEAR(currentAcceleration.z, 0.f, 0.01f);

    physicalModel_free(model);
}

TEST(PhysicalModel, NonInstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    PhysicalModel* model = physicalModel_new();

    vec3 startRotation;
    startRotation.x = 0.f;
    startRotation.y = 0.f;
    startRotation.z = 0.f;
    physicalModel_setTargetRotation(
            model, startRotation, PHYSICAL_INTERPOLATION_STEP);

    mTestSystem.setUnixTime(1);
    vec3 newRotation;
    newRotation.x = 180.0f;
    newRotation.y = 0.0f;
    newRotation.z = 0.0f;
    physicalModel_setTargetRotation(
            model, newRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    long measurement_id;
    vec3 currentGyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_LE(currentGyro.x, -0.01f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, InstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    PhysicalModel* model = physicalModel_new();

    vec3 startRotation;
    startRotation.x = 0.f;
    startRotation.y = 0.f;
    startRotation.z = 0.f;
    physicalModel_setTargetRotation(
            model, startRotation, PHYSICAL_INTERPOLATION_STEP);

    mTestSystem.setUnixTime(1);
    vec3 newRotation;
    newRotation.x = 180.0f;
    newRotation.y = 0.0f;
    newRotation.z = 0.0f;
    physicalModel_setTargetRotation(
            model, newRotation, PHYSICAL_INTERPOLATION_STEP);

    long measurement_id;
    vec3 currentGyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_NEAR(currentGyro.x, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, OverrideAccelerometer) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(0);
    PhysicalModel* model = physicalModel_new();

    long initial_measurement_id;
    physicalModel_getAccelerometer(model, &initial_measurement_id);

    vec3 overrideValue;
    overrideValue.x = 1.f;
    overrideValue.y = 2.f;
    overrideValue.z = 3.f;
    physicalModel_overrideAccelerometer(model, overrideValue);

    long override_measurement_id;
    vec3 sensorOverriddenValue = physicalModel_getAccelerometer(model,
            &override_measurement_id);
    EXPECT_EQ(overrideValue.x, sensorOverriddenValue.x);
    EXPECT_EQ(overrideValue.y, sensorOverriddenValue.y);
    EXPECT_EQ(overrideValue.z, sensorOverriddenValue.z);

    EXPECT_NE(initial_measurement_id, override_measurement_id);

    vec3 targetPosition;
    targetPosition.x = 0.f;
    targetPosition.y = 0.f;
    targetPosition.z = 0.f;
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_STEP);

    long physical_measurement_id;
    vec3 sensorPhysicalValue = physicalModel_getAccelerometer(model,
            &physical_measurement_id);
    EXPECT_NEAR(0.f, sensorPhysicalValue.x, 0.000001f);
    EXPECT_NEAR(9.81f, sensorPhysicalValue.y, 0.000001f);
    EXPECT_NEAR(0.f, sensorPhysicalValue.z, 0.000001f);

    EXPECT_NE(physical_measurement_id, override_measurement_id);
    EXPECT_NE(physical_measurement_id, initial_measurement_id);

    physicalModel_free(model);
}
