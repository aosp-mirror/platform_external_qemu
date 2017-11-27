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

#include "android/utils/stream.h"
#include "android/base/files/MemStream.h"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;

static Stream* asStream(android::base::Stream* stream) {
    return reinterpret_cast<Stream*>(stream);
}

#define EXPECT_VEC3_NEAR(e,a,d)\
EXPECT_NEAR(e.x,a.x,d);\
EXPECT_NEAR(e.y,a.y,d);\
EXPECT_NEAR(e.z,a.z,d);

TEST(PhysicalModel, CreateAndDestroy) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new(false);
    EXPECT_NE(model, nullptr);
    physicalModel_free(model);
}

TEST(PhysicalModel, DefaultInertialSensorValues) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 1000000000L);
    long measurement_id;
    vec3 accelerometer = physicalModel_getAccelerometer(model,
            &measurement_id);
    EXPECT_VEC3_NEAR((vec3{0.f, 9.81f, 0.f}), accelerometer, 0.001f);

    vec3 gyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_VEC3_NEAR((vec3{0.f, 0.f, 0.f}), gyro, 0.001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, ConstantMeasurementId) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 1000000000L);
    long measurement_id0;
    physicalModel_getAccelerometer(model, &measurement_id0);

    physicalModel_setCurrentTime(model, 2000000000L);

    long measurement_id1;
    physicalModel_getAccelerometer(model, &measurement_id1);

    EXPECT_EQ(measurement_id0, measurement_id1);

    physicalModel_free(model);
}


TEST(PhysicalModel, NewMeasurementId) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 1000000000L);
    long measurement_id0;
    physicalModel_getAccelerometer(model, &measurement_id0);

    physicalModel_setCurrentTime(model, 2000000000L);

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
    PhysicalModel *model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 0UL);
    vec3 targetPosition;
    targetPosition.x = 2.0f;
    targetPosition.y = 3.0f;
    targetPosition.z = 4.0f;
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_STEP);

    physicalModel_setCurrentTime(model, 500000000L);

    vec3 currentPosition = physicalModel_getParameterPosition(model,
            PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_VEC3_NEAR(targetPosition, currentPosition, 0.0001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, SetTargetRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel *model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 0UL);
    vec3 targetRotation;
    targetRotation.x = 45.0f;
    targetRotation.y = 10.0f;
    targetRotation.z = 4.0f;
    physicalModel_setTargetRotation(
            model, targetRotation, PHYSICAL_INTERPOLATION_STEP);

    physicalModel_setCurrentTime(model, 500000000L);
    vec3 currentRotation = physicalModel_getParameterRotation(model,
            PARAMETER_VALUE_TYPE_CURRENT);

    EXPECT_VEC3_NEAR(targetRotation, currentRotation, 0.0001f);

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
    for (const auto& testCase : gravityTestCases) {
        PhysicalModel* model = physicalModel_new(false);
        physicalModel_setCurrentTime(model, 1000000000L);

        vec3 targetRotation;
        targetRotation.x = testCase.target_rotation.x;
        targetRotation.y = testCase.target_rotation.y;
        targetRotation.z = testCase.target_rotation.z;

        physicalModel_setTargetRotation(
                model, targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);

        physicalModel_setCurrentTime(model, 2000000000L);;

        long measurement_id;
        vec3 accelerometer = physicalModel_getAccelerometer(model,
                &measurement_id);

        EXPECT_VEC3_NEAR(testCase.expected_acceleration, accelerometer, 0.01f);

        physicalModel_free(model);
    }
}

TEST(PhysicalModel, GravityOnlyAcceleration) {
    TestSystem mTestSystem("/", System::kProgramBitness);

    PhysicalModel* model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 1000000000L);

    vec3 targetPosition;
    targetPosition.x = 2.0f;
    targetPosition.y = 3.0f;
    targetPosition.z = 4.0f;
    // at 1 second we move the target to (2, 3, 4)
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);

    physicalModel_setCurrentTime(model, 2000000000L);
    // at 2 seconds the target is still at (2, 3, 4);
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_STEP);

    long measurement_id;
    // the acceleration is expected to be close to zero at this point.
    vec3 currentAcceleration = physicalModel_getAccelerometer(model,
            &measurement_id);
    EXPECT_VEC3_NEAR((vec3{0.f, 9.81f, 0.f}), currentAcceleration, 0.01f);

    physicalModel_free(model);
}

TEST(PhysicalModel, NonInstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 0L);

    vec3 startRotation;
    startRotation.x = 0.f;
    startRotation.y = 0.f;
    startRotation.z = 0.f;
    physicalModel_setTargetRotation(
            model, startRotation, PHYSICAL_INTERPOLATION_STEP);

    physicalModel_setCurrentTime(model, 1000000000L);
    vec3 newRotation;
    newRotation.x = -0.5f;
    newRotation.y = 0.0f;
    newRotation.z = 0.0f;
    physicalModel_setTargetRotation(
            model, newRotation, PHYSICAL_INTERPOLATION_SMOOTH);

    physicalModel_setCurrentTime(model, 1125000000L);

    long measurement_id;
    vec3 currentGyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_LE(currentGyro.x, -0.01f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, InstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 0L);

    vec3 startRotation;
    startRotation.x = 0.f;
    startRotation.y = 0.f;
    startRotation.z = 0.f;
    physicalModel_setTargetRotation(
            model, startRotation, PHYSICAL_INTERPOLATION_STEP);

    physicalModel_setCurrentTime(model, 1000000000L);
    vec3 newRotation;
    newRotation.x = 180.0f;
    newRotation.y = 0.0f;
    newRotation.z = 0.0f;
    physicalModel_setTargetRotation(
            model, newRotation, PHYSICAL_INTERPOLATION_STEP);

    long measurement_id;
    vec3 currentGyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_VEC3_NEAR((vec3 {0.f, 0.f, 0.f}), currentGyro, 0.000001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, OverrideAccelerometer) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);
    physicalModel_setCurrentTime(model, 0L);

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
    EXPECT_VEC3_NEAR(overrideValue, sensorOverriddenValue, 0.000001f);

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
    EXPECT_VEC3_NEAR((vec3 {0.f, 9.81f, 0.f}), sensorPhysicalValue, 0.000001f);

    EXPECT_NE(physical_measurement_id, override_measurement_id);
    EXPECT_NE(physical_measurement_id, initial_measurement_id);

    physicalModel_free(model);
}

TEST(PhysicalModel, SaveLoad) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_save(model, saveStream);

    const uint32_t streamEndMarker = 1923789U;
    stream_put_be32(saveStream, streamEndMarker);

    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new(false);
    physicalModel_load(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadOverrides) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);

    const vec3 accelOverride = {1.f, 2.f, 3.f};
    const vec3 gyroOverride = {4.f, 5.f, 6.f};
    const vec3 magnetometerOverride = {7.f, 8.f, 9.f};
    const vec3 orientationOverride = {10.f, 11.f, 12.f};
    const float temperatureOverride = 13.f;
    const float proximityOverride = 14.f;
    const float lightOverride = 15.f;
    const float pressureOverride = 16.f;
    const float humidityOverride = 17.f;
    const vec3 magneticUncalibratedOverride = {18.f, 19.f, 20.f};
    const vec3 gyroUncalibratedOverride = {21.f, 22.f, 23.f};

    physicalModel_overrideAccelerometer(model, accelOverride);
    physicalModel_overrideGyroscope(model, gyroOverride);
    physicalModel_overrideMagnetometer(model, magnetometerOverride);
    physicalModel_overrideOrientation(model, orientationOverride);
    physicalModel_overrideTemperature(model, temperatureOverride);
    physicalModel_overrideProximity(model, proximityOverride);
    physicalModel_overrideLight(model, lightOverride);
    physicalModel_overridePressure(model, pressureOverride);
    physicalModel_overrideHumidity(model, humidityOverride);
    physicalModel_overrideMagnetometerUncalibrated(model,
            magneticUncalibratedOverride);
    physicalModel_overrideGyroscopeUncalibrated(model,
            gyroUncalibratedOverride);

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_save(model, saveStream);
    physicalModel_free(model);

    const uint32_t streamEndMarker = 349087U;
    stream_put_be32(saveStream, streamEndMarker);

    PhysicalModel* loadedModel = physicalModel_new(false);
    physicalModel_load(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    long measurement_id;
    EXPECT_VEC3_NEAR(accelOverride,
            physicalModel_getAccelerometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(gyroOverride,
            physicalModel_getGyroscope(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(magnetometerOverride,
            physicalModel_getMagnetometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(orientationOverride,
            physicalModel_getOrientation(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_NEAR(temperatureOverride,
            physicalModel_getTemperature(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_NEAR(proximityOverride,
            physicalModel_getProximity(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_NEAR(lightOverride,
            physicalModel_getLight(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_NEAR(pressureOverride,
            physicalModel_getPressure(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_NEAR(humidityOverride,
            physicalModel_getHumidity(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(magneticUncalibratedOverride,
            physicalModel_getMagnetometerUncalibrated(
                    loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(gyroUncalibratedOverride,
            physicalModel_getGyroscopeUncalibrated(
                    loadedModel, &measurement_id),
            0.00001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadTargets) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new(false);

    const vec3 positionTarget = {24.f, 25.f, 26.f};
    const vec3 rotationTarget = {27.f, 28.f, 29.f};
    const vec3 magneticFieldTarget = {30.f, 31.f, 32.f};
    const float temperatureTarget = 33.f;
    const float proximityTarget = 34.f;
    const float lightTarget = 35.f;
    const float pressureTarget = 36.f;
    const float humidityTarget = 37.f;

    // Note: Save/Load should save out target state - interpolation mode should
    //       not be used, nor relevant (i.e. when loading, the interpolation is
    //       considered to have finisshed).
    physicalModel_setTargetPosition(model, positionTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetRotation(model, rotationTarget,
            PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetMagneticField(model, magneticFieldTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetTemperature(model, temperatureTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetProximity(model, proximityTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetLight(model, lightTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetPressure(model, pressureTarget,
            PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetHumidity(model, humidityTarget,
            PHYSICAL_INTERPOLATION_STEP);

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_save(model, saveStream);
    physicalModel_free(model);

    const uint32_t streamEndMarker = 3489U;
    stream_put_be32(saveStream, streamEndMarker);

    PhysicalModel* loadedModel = physicalModel_new(false);
    physicalModel_load(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    EXPECT_VEC3_NEAR(positionTarget,
            physicalModel_getParameterPosition(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_VEC3_NEAR(rotationTarget,
            physicalModel_getParameterRotation(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_VEC3_NEAR(magneticFieldTarget,
            physicalModel_getParameterMagneticField(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_NEAR(temperatureTarget,
            physicalModel_getParameterTemperature(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_NEAR(proximityTarget,
            physicalModel_getParameterProximity(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_NEAR(lightTarget,
            physicalModel_getParameterLight(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_NEAR(pressureTarget,
            physicalModel_getParameterPressure(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);
    EXPECT_NEAR(humidityTarget,
            physicalModel_getParameterHumidity(loadedModel,
                                               PARAMETER_VALUE_TYPE_TARGET),
            0.00001f);

    physicalModel_free(loadedModel);
}
