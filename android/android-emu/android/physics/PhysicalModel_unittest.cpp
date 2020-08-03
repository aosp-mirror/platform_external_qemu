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

#include "automation.pb.h"
#include "android/base/Debug.h"
#include "android/base/files/MemStream.h"
#include "android/base/testing/GlmTestHelpers.h"
#include "android/base/testing/ProtobufMatchers.h"
#include "android/globals.h"
#include "android/physics/InertialModel.h"
#include "android/physics/physical_state_agent.h"
#include "android/utils/stream.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;

static constexpr float kDefaultProximity = 1.f;
static constexpr vec3 kDefaultAccelerometer = {0.f, 9.81f, 0.f};
static constexpr vec3 kDefaultMagnetometer = {0.0f, 5.9f, -48.4f};

static Stream* asStream(android::base::Stream* stream) {
    return reinterpret_cast<Stream*>(stream);
}

#define EXPECT_VEC3_NEAR(e,a,d)\
EXPECT_NEAR(e.x,a.x,d);\
EXPECT_NEAR(e.y,a.y,d);\
EXPECT_NEAR(e.z,a.z,d);

TEST(PhysicalModel, CreateAndDestroy) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
    EXPECT_NE(model, nullptr);
    physicalModel_free(model);
}

TEST(PhysicalModel, DefaultInertialSensorValues) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
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
    PhysicalModel* model = physicalModel_new();
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
    PhysicalModel* model = physicalModel_new();
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
    PhysicalModel* model = physicalModel_new();
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
    PhysicalModel* model = physicalModel_new();
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
        PhysicalModel* model = physicalModel_new();
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

    PhysicalModel* model = physicalModel_new();
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
    EXPECT_VEC3_NEAR(kDefaultAccelerometer, currentAcceleration, 0.01f);

    physicalModel_free(model);
}

TEST(PhysicalModel, NonInstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
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

    physicalModel_setCurrentTime(model, 1000000000L +
            android::physics::secondsToNs(
                    android::physics::kMinStateChangeTimeSeconds / 2.f));

    long measurement_id;
    vec3 currentGyro = physicalModel_getGyroscope(model, &measurement_id);
    EXPECT_LE(currentGyro.x, -0.01f);
    EXPECT_NEAR(currentGyro.y, 0.0, 0.000001f);
    EXPECT_NEAR(currentGyro.z, 0.0, 0.000001f);

    physicalModel_free(model);
}

TEST(PhysicalModel, InstantaneousRotation) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
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
    PhysicalModel* model = physicalModel_new();
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
    EXPECT_VEC3_NEAR(kDefaultAccelerometer, sensorPhysicalValue, 0.000001f);

    EXPECT_NE(physical_measurement_id, override_measurement_id);
    EXPECT_NE(physical_measurement_id, initial_measurement_id);

    physicalModel_free(model);
}

TEST(PhysicalModel, SaveLoad) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_snapshotSave(model, saveStream);

    const uint32_t streamEndMarker = 1923789U;
    stream_put_be32(saveStream, streamEndMarker);

    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_snapshotLoad(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadStateSimple) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    emulator_automation::InitialState state;
    physicalModel_saveState(model, &state);

    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_loadState(loadedModel, state);

    physicalModel_free(loadedModel);
}

static constexpr vec3 kVecZero = {0.f, 0.f, 0.f};
static constexpr vec3 kAccelOverride = {1.f, 2.f, 3.f};
static constexpr vec3 kGyroOverride = {4.f, 5.f, 6.f};
static constexpr vec3 kMagnetometerOverride = {7.f, 8.f, 9.f};
static constexpr vec3 kOrientationOverride = {10.f, 11.f, 12.f};
static constexpr float kTemperatureOverride = 13.f;
static constexpr float kProximityOverride = 14.f;
static constexpr float kLightOverride = 15.f;
static constexpr float kPressureOverride = 16.f;
static constexpr float kHumidityOverride = 17.f;
static constexpr vec3 kMagneticUncalibratedOverride = {18.f, 19.f, 20.f};
static constexpr vec3 kGyroUncalibratedOverride = {21.f, 22.f, 23.f};

TEST(PhysicalModel, SaveLoadStateDefaultValues) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    emulator_automation::InitialState state;
    physicalModel_saveState(model, &state);

    physicalModel_free(model);

    // Initial state should not contain any values if everything is in a default
    // state.
    EXPECT_THAT(state,
                android::EqualsProto(
                        emulator_automation::InitialState::default_instance()));
    EXPECT_EQ(state.ByteSize(), 0);

    PhysicalModel* loadedModel = physicalModel_new();

    static constexpr vec3 kPosition = {1.f, 2.f, 3.f};
    physicalModel_setTargetPosition(loadedModel, kPosition,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_loadState(loadedModel, state);

    // Validate that the position parameter was reset when loading state.
    vec3 loadedPosition = physicalModel_getParameterPosition(
            loadedModel, PARAMETER_VALUE_TYPE_TARGET);
    EXPECT_VEC3_NEAR(loadedPosition, kVecZero, 0.000001f);

    physicalModel_free(loadedModel);
}

static void applyOverrides(PhysicalModel* model) {
    physicalModel_overrideAccelerometer(model, kAccelOverride);
    physicalModel_overrideGyroscope(model, kGyroOverride);
    physicalModel_overrideMagnetometer(model, kMagnetometerOverride);
    physicalModel_overrideOrientation(model, kOrientationOverride);
    physicalModel_overrideTemperature(model, kTemperatureOverride);
    physicalModel_overrideProximity(model, kProximityOverride);
    physicalModel_overrideLight(model, kLightOverride);
    physicalModel_overridePressure(model, kPressureOverride);
    physicalModel_overrideHumidity(model, kHumidityOverride);
    physicalModel_overrideMagnetometerUncalibrated(
            model, kMagneticUncalibratedOverride);
    physicalModel_overrideGyroscopeUncalibrated(model,
                                                kGyroUncalibratedOverride);
}

TEST(PhysicalModel, SaveLoadOverrides) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    applyOverrides(model);

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_snapshotSave(model, saveStream);
    physicalModel_free(model);

    const uint32_t streamEndMarker = 349087U;
    stream_put_be32(saveStream, streamEndMarker);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_snapshotLoad(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    long measurement_id;
    EXPECT_VEC3_NEAR(
            kAccelOverride,
            physicalModel_getAccelerometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(kGyroOverride,
                     physicalModel_getGyroscope(loadedModel, &measurement_id),
                     0.00001f);
    EXPECT_VEC3_NEAR(
            kMagnetometerOverride,
            physicalModel_getMagnetometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(kOrientationOverride,
                     physicalModel_getOrientation(loadedModel, &measurement_id),
                     0.00001f);
    EXPECT_NEAR(kTemperatureOverride,
                physicalModel_getTemperature(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(kProximityOverride,
                physicalModel_getProximity(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(kLightOverride,
                physicalModel_getLight(loadedModel, &measurement_id), 0.00001f);
    EXPECT_NEAR(kPressureOverride,
                physicalModel_getPressure(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(kHumidityOverride,
                physicalModel_getHumidity(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_VEC3_NEAR(kMagneticUncalibratedOverride,
                     physicalModel_getMagnetometerUncalibrated(loadedModel,
                                                               &measurement_id),
                     0.00001f);
    EXPECT_VEC3_NEAR(kGyroUncalibratedOverride,
                     physicalModel_getGyroscopeUncalibrated(loadedModel,
                                                            &measurement_id),
                     0.00001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadStateOverrides) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    applyOverrides(model);

    emulator_automation::InitialState state;
    physicalModel_saveState(model, &state);
    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_loadState(loadedModel, state);

    // loadState doesn't load overrides, verify they all are zero.
    long measurement_id;
    EXPECT_VEC3_NEAR(
            kDefaultAccelerometer,
            physicalModel_getAccelerometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(kVecZero,
                     physicalModel_getGyroscope(loadedModel, &measurement_id),
                     0.00001f);
    EXPECT_VEC3_NEAR(
            kDefaultMagnetometer,
            physicalModel_getMagnetometer(loadedModel, &measurement_id),
            0.00001f);
    EXPECT_VEC3_NEAR(kVecZero,
                     physicalModel_getOrientation(loadedModel, &measurement_id),
                     0.00001f);
    EXPECT_NEAR(0.f, physicalModel_getTemperature(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(kDefaultProximity,
                physicalModel_getProximity(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(0.f, physicalModel_getLight(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(0.f, physicalModel_getPressure(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_NEAR(0.f, physicalModel_getHumidity(loadedModel, &measurement_id),
                0.00001f);
    EXPECT_VEC3_NEAR(kVecZero,
                     physicalModel_getGyroscopeUncalibrated(loadedModel,
                                                            &measurement_id),
                     0.00001f);
    EXPECT_VEC3_NEAR(kDefaultMagnetometer,
                     physicalModel_getMagnetometerUncalibrated(loadedModel,
                                                               &measurement_id),
                     0.00001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadTargets) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    const vec3 kPositionTarget = {24.f, 25.f, 26.f};
    const vec3 kRotationTarget = {27.f, 28.f, 29.f};
    const vec3 kMagneticFieldTarget = {30.f, 31.f, 32.f};
    const float kTemperatureTarget = 33.f;
    const float kProximityTarget = 34.f;
    const float kLightTarget = 35.f;
    const float kPressureTarget = 36.f;
    const float kHumidityTarget = 37.f;

    // Note: Save/Load should save out target state - interpolation mode should
    //       not be used, nor relevant (i.e. when loading, the interpolation is
    //       considered to have finisshed).
    physicalModel_setTargetPosition(model, kPositionTarget,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetRotation(model, kRotationTarget,
                                    PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetMagneticField(model, kMagneticFieldTarget,
                                         PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetTemperature(model, kTemperatureTarget,
                                       PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetProximity(model, kProximityTarget,
                                     PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetLight(model, kLightTarget,
                                 PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetPressure(model, kPressureTarget,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetHumidity(model, kHumidityTarget,
                                    PHYSICAL_INTERPOLATION_STEP);

    android::base::MemStream modelStream;
    Stream* saveStream = asStream(&modelStream);

    physicalModel_snapshotSave(model, saveStream);
    physicalModel_free(model);

    const uint32_t streamEndMarker = 3489U;
    stream_put_be32(saveStream, streamEndMarker);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_snapshotLoad(loadedModel, saveStream);

    EXPECT_EQ(streamEndMarker, stream_get_be32(saveStream));

    EXPECT_VEC3_NEAR(kPositionTarget,
                     physicalModel_getParameterPosition(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.00001f);
    EXPECT_VEC3_NEAR(kRotationTarget,
                     physicalModel_getParameterRotation(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.0001f);
    EXPECT_VEC3_NEAR(kMagneticFieldTarget,
                     physicalModel_getParameterMagneticField(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.00001f);
    EXPECT_NEAR(kTemperatureTarget,
                physicalModel_getParameterTemperature(
                        loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                0.00001f);
    EXPECT_NEAR(kProximityTarget,
                physicalModel_getParameterProximity(
                        loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                0.00001f);
    EXPECT_NEAR(kLightTarget,
                physicalModel_getParameterLight(loadedModel,
                                                PARAMETER_VALUE_TYPE_TARGET),
                0.00001f);
    EXPECT_NEAR(kPressureTarget,
                physicalModel_getParameterPressure(loadedModel,
                                                   PARAMETER_VALUE_TYPE_TARGET),
                0.00001f);
    EXPECT_NEAR(kHumidityTarget,
                physicalModel_getParameterHumidity(loadedModel,
                                                   PARAMETER_VALUE_TYPE_TARGET),
                0.00001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadStatePosition) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    const vec3 kPositionCurrent = {-1.f, 21.f, -6.f};
    const vec3 kRotationCurrent = {5.f, 35.f, -5.f};
    const vec3 kPositionTarget = {24.f, 25.f, 26.f};
    const vec3 kRotationTarget = {27.f, 28.f, 29.f};

    // NOTE: saveState/loadState should save out both current and target state.
    physicalModel_setTargetPosition(model, kPositionCurrent,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetPosition(model, kPositionTarget,
                                    PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetRotation(model, kRotationCurrent,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetRotation(model, kRotationTarget,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    emulator_automation::InitialState state;
    physicalModel_saveState(model, &state);
    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_loadState(loadedModel, state);

    EXPECT_VEC3_NEAR(kPositionCurrent,
                     physicalModel_getParameterPosition(
                             loadedModel,
                             PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION),
                     0.00001f);
    EXPECT_VEC3_NEAR(kPositionTarget,
                     physicalModel_getParameterPosition(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.00001f);
    EXPECT_VEC3_NEAR(kRotationCurrent,
                     physicalModel_getParameterRotation(
                             loadedModel,
                             PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION),
                     0.0001f);
    EXPECT_VEC3_NEAR(kRotationTarget,
                     physicalModel_getParameterRotation(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.0001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SaveLoadStateVelocity) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();

    const vec3 kPositionCurrent = {-1.f, 21.f, -6.f};
    const vec3 kVelocityCurrent = {5.f, 35.f, -5.f};
    const vec3 kPositionTarget = {24.f, 25.f, 26.f};
    const vec3 kVelocityTarget = {27.f, 28.f, 29.f};

    physicalModel_setTargetPosition(model, kPositionCurrent,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetPosition(model, kPositionTarget,
                                    PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetVelocity(model, kVelocityCurrent,
                                    PHYSICAL_INTERPOLATION_STEP);
    physicalModel_setTargetVelocity(model, kVelocityTarget,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    emulator_automation::InitialState state;
    physicalModel_saveState(model, &state);
    physicalModel_free(model);

    PhysicalModel* loadedModel = physicalModel_new();
    physicalModel_loadState(loadedModel, state);

    EXPECT_VEC3_NEAR(kPositionCurrent,
                     physicalModel_getParameterPosition(
                             loadedModel,
                             PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION),
                     0.00001f);
    // NOTE: When velocity is set target position is no longer valid.
    EXPECT_VEC3_NEAR(kVelocityCurrent,
                     physicalModel_getParameterVelocity(
                             loadedModel,
                             PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION),
                     0.0001f);
    EXPECT_VEC3_NEAR(kVelocityTarget,
                     physicalModel_getParameterVelocity(
                             loadedModel, PARAMETER_VALUE_TYPE_TARGET),
                     0.0001f);

    physicalModel_free(loadedModel);
}

TEST(PhysicalModel, SetRotatedIMUResults) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
    physicalModel_setCurrentTime(model, 0UL);

    const vec3 initialRotation {45.0f, 10.0f, 4.0f};

    physicalModel_setTargetRotation(
            model, initialRotation, PHYSICAL_INTERPOLATION_STEP);

    const vec3 initialPosition {2.0f, 3.0f, 4.0f};
    physicalModel_setTargetPosition(
            model, initialPosition, PHYSICAL_INTERPOLATION_STEP);

    const glm::quat quaternionRotation = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(initialRotation.x),
            glm::radians(initialRotation.y),
            glm::radians(initialRotation.z)));

    uint64_t time = 500000000UL;
    const uint64_t stepNs = 1000UL;
    const uint64_t maxConvergenceTimeNs =
            time + android::physics::secondsToNs(5.0f);

    const vec3 targetPosition {1.0f, 2.0f, 3.0f};

    static int testContext;
    static bool targetStateChanged = false;
    static bool physicalStateChanging = false;
    const QAndroidPhysicalStateAgent agent = {
        .onTargetStateChanged = [](void* context) {
            EXPECT_EQ(context, &testContext);
            targetStateChanged = true;
        },
        .onPhysicalStateChanging = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = true;
        },
        .onPhysicalStateStabilized = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = false;
        },
        .context = &testContext};

    physicalModel_setCurrentTime(model, time);
    physicalModel_setPhysicalStateAgent(model, &agent);
    EXPECT_FALSE(physicalStateChanging);
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);
    EXPECT_TRUE(targetStateChanged);
    EXPECT_TRUE(physicalStateChanging);
    targetStateChanged = false;

    const glm::vec3 gravity(0.f, 9.81f, 0.f);

    glm::vec3 velocity(0.f);
    glm::vec3 position(initialPosition.x, initialPosition.y, initialPosition.z);
    const float stepSeconds = android::physics::nsToSeconds(stepNs);
    long prevMeasurementId = -1;
    time += stepNs / 2;

    size_t iteration = 0;
    while (physicalStateChanging) {
        SCOPED_TRACE(testing::Message() << "Iteration " << iteration);
        ++iteration;

        ASSERT_LT(time, maxConvergenceTimeNs)
                << "Physical state did not stabilize";
        physicalModel_setCurrentTime(model, time);
        long measurementId;
        const vec3 measuredAcceleration = physicalModel_getAccelerometer(
                model, &measurementId);
        ASSERT_NE(prevMeasurementId, measurementId);
        prevMeasurementId = measurementId;
        const glm::vec3 acceleration(
                measuredAcceleration.x,
                measuredAcceleration.y,
                measuredAcceleration.z);
        velocity += (quaternionRotation * acceleration - gravity) *
                stepSeconds;
        position += velocity * stepSeconds;
        time += stepNs;
    }

    const vec3 integratedPosition {position.x, position.y, position.z};

    EXPECT_VEC3_NEAR(targetPosition, integratedPosition, 0.01f);

    EXPECT_FALSE(targetStateChanged);
    physicalModel_setPhysicalStateAgent(model, nullptr);

    physicalModel_free(model);
}

TEST(PhysicalModel, SetRotationIMUResults) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
    physicalModel_setCurrentTime(model, 0UL);

    vec3 initialRotation {45.0f, 10.0f, 4.0f};

    physicalModel_setTargetRotation(
            model, initialRotation, PHYSICAL_INTERPOLATION_STEP);

    uint64_t time = 0UL;
    const uint64_t stepNs = 5000UL;
    const uint64_t maxConvergenceTimeNs =
            time + android::physics::secondsToNs(5.0f);

    vec3 targetRotation {-10.0f, 20.0f, 45.0f};

    static int testContext;
    static bool targetStateChanged = false;
    static bool physicalStateChanging = false;
    const QAndroidPhysicalStateAgent agent = {
        .onTargetStateChanged = [](void* context) {
            EXPECT_EQ(context, &testContext);
            targetStateChanged = true;
        },
        .onPhysicalStateChanging = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = true;
        },
        .onPhysicalStateStabilized = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = false;
        },
        .context = &testContext};
    physicalModel_setPhysicalStateAgent(model, &agent);

    physicalModel_setTargetRotation(
            model, targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);
    EXPECT_TRUE(targetStateChanged);
    EXPECT_TRUE(physicalStateChanging);
    targetStateChanged = false;

    glm::quat rotation = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(initialRotation.x),
            glm::radians(initialRotation.y),
            glm::radians(initialRotation.z)));
    const float stepSeconds = android::physics::nsToSeconds(stepNs);
    long prevMeasurementId = -1;
    time += stepNs / 2;
    while (physicalStateChanging) {
        ASSERT_LT(time, maxConvergenceTimeNs)
                << "Physical state did not stabilize";
        physicalModel_setCurrentTime(model, time);
        long measurementId;
        const vec3 measuredGyroscope = physicalModel_getGyroscope(
                model, &measurementId);
        ASSERT_NE(prevMeasurementId, measurementId);
        prevMeasurementId = measurementId;
        const glm::vec3 deviceSpaceRotationalVelocity(
                measuredGyroscope.x,
                measuredGyroscope.y,
                measuredGyroscope.z);
        const glm::vec3 rotationalVelocity =
                rotation * deviceSpaceRotationalVelocity;
        const glm::mat4 deltaRotationMatrix = glm::eulerAngleXYZ(
                rotationalVelocity.x * stepSeconds,
                rotationalVelocity.y * stepSeconds,
                rotationalVelocity.z * stepSeconds);

        rotation = glm::quat_cast(deltaRotationMatrix) * rotation;
        time += stepNs;
    }

    const glm::quat targetRotationQuat = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(targetRotation.x),
            glm::radians(targetRotation.y),
            glm::radians(targetRotation.z)));

    EXPECT_NEAR(targetRotationQuat.x, rotation.x, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.y, rotation.y, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.z, rotation.z, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.w, rotation.w, 0.0001f);

    EXPECT_FALSE(targetStateChanged);
    physicalModel_setPhysicalStateAgent(model, nullptr);

    physicalModel_free(model);
}

TEST(PhysicalModel, MoveWhileRotating) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
    physicalModel_setCurrentTime(model, 0UL);

    const vec3 initialRotation {45.0f, 10.0f, 4.0f};

    physicalModel_setTargetRotation(
            model, initialRotation, PHYSICAL_INTERPOLATION_STEP);

    const vec3 initialPosition {2.0f, 3.0f, 4.0f};
    physicalModel_setTargetPosition(
            model, initialPosition, PHYSICAL_INTERPOLATION_STEP);

    uint64_t time = 0UL;
    const uint64_t stepNs = 5000UL;
    const uint64_t maxConvergenceTimeNs =
            time + android::physics::secondsToNs(5.0f);

    const vec3 targetPosition {1.0f, 2.0f, 3.0f};
    const vec3 targetRotation {-10.0f, 20.0f, 45.0f};

    static int testContext;
    static bool targetStateChanged = false;
    static bool physicalStateChanging = false;
    const QAndroidPhysicalStateAgent agent = {
        .onTargetStateChanged = [](void* context) {
            EXPECT_EQ(context, &testContext);
            targetStateChanged = true;
        },
        .onPhysicalStateChanging = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = true;
        },
        .onPhysicalStateStabilized = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = false;
        },
        .context = &testContext};
    physicalModel_setPhysicalStateAgent(model, &agent);

    physicalModel_setTargetRotation(
            model, targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetPosition(
            model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);
    EXPECT_TRUE(targetStateChanged);
    EXPECT_TRUE(physicalStateChanging);
    targetStateChanged = false;

    glm::quat rotation = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(initialRotation.x),
            glm::radians(initialRotation.y),
            glm::radians(initialRotation.z)));
    const glm::vec3 gravity(0.f, 9.81f, 0.f);
    glm::vec3 velocity(0.f);
    glm::vec3 position(initialPosition.x, initialPosition.y, initialPosition.z);

    const float stepSeconds = android::physics::nsToSeconds(stepNs);
    long prevGyroMeasurementId = -1;
    long prevAccelMeasurementId = -1;
    time += stepNs / 2;
    while (physicalStateChanging) {
        ASSERT_LT(time, maxConvergenceTimeNs)
                << "Physical state did not stabilize";
        physicalModel_setCurrentTime(model, time);
        long gyroMeasurementId;
        const vec3 measuredGyroscope = physicalModel_getGyroscope(
                model, &gyroMeasurementId);
        ASSERT_NE(prevGyroMeasurementId, gyroMeasurementId);
        prevGyroMeasurementId = gyroMeasurementId;
        const glm::vec3 deviceSpaceRotationalVelocity(
                measuredGyroscope.x,
                measuredGyroscope.y,
                measuredGyroscope.z);
        const glm::vec3 rotationalVelocity =
                rotation * deviceSpaceRotationalVelocity;
        const glm::mat4 deltaRotationMatrix = glm::eulerAngleXYZ(
                rotationalVelocity.x * stepSeconds,
                rotationalVelocity.y * stepSeconds,
                rotationalVelocity.z * stepSeconds);

        rotation = glm::quat_cast(deltaRotationMatrix) * rotation;

        long accelMeasurementId;
        const vec3 measuredAcceleration = physicalModel_getAccelerometer(
                model, &accelMeasurementId);
        EXPECT_NE(prevAccelMeasurementId, accelMeasurementId);
        prevAccelMeasurementId = accelMeasurementId;
        const glm::vec3 acceleration(measuredAcceleration.x,
                                     measuredAcceleration.y,
                                     measuredAcceleration.z);
        velocity += (rotation * acceleration - gravity) *
                stepSeconds;
        position += velocity * stepSeconds;

        time += stepNs;
    }

    const glm::quat targetRotationQuat = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(targetRotation.x),
            glm::radians(targetRotation.y),
            glm::radians(targetRotation.z)));

    EXPECT_NEAR(targetRotationQuat.x, rotation.x, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.y, rotation.y, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.z, rotation.z, 0.0001f);
    EXPECT_NEAR(targetRotationQuat.w, rotation.w, 0.0001f);

    vec3 integratedPosition {position.x, position.y, position.z};

    EXPECT_VEC3_NEAR(targetPosition, integratedPosition, 0.001f);

    EXPECT_FALSE(targetStateChanged);
    physicalModel_setPhysicalStateAgent(model, nullptr);

    physicalModel_free(model);
}


TEST(PhysicalModel, SetVelocityAndPositionWhileRotating) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    PhysicalModel* model = physicalModel_new();
    physicalModel_setCurrentTime(model, 0UL);

    const vec3 initialRotation {45.0f, 10.0f, 4.0f};

    physicalModel_setTargetRotation(
            model, initialRotation, PHYSICAL_INTERPOLATION_STEP);

    const vec3 initialPosition {2.0f, 3.0f, 4.0f};
    physicalModel_setTargetPosition(
            model, initialPosition, PHYSICAL_INTERPOLATION_STEP);

    vec3 intermediateVelocity {1.0f, 1.0f, 1.0f};

    uint64_t time = 0UL;
    const uint64_t stepNs = 5000UL;

    const vec3 targetPosition {1.0f, 2.0f, 3.0f};

    const vec3 intermediateRotation {100.0f, -30.0f, -10.0f};

    const vec3 targetRotation {-10.0f, 20.0f, 45.0f};

    static int testContext;
    static bool targetStateChanged = false;
    static bool physicalStateChanging = false;
    const QAndroidPhysicalStateAgent agent = {
        .onTargetStateChanged = [](void* context) {
            EXPECT_EQ(context, &testContext);
            targetStateChanged = true;
        },
        .onPhysicalStateChanging = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = true;
        },
        .onPhysicalStateStabilized = [](void* context) {
            EXPECT_EQ(context, &testContext);
            physicalStateChanging = false;
        },
        .context = &testContext};
    physicalModel_setPhysicalStateAgent(model, &agent);

    physicalModel_setTargetRotation(
            model, intermediateRotation, PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetVelocity(
            model, intermediateVelocity, PHYSICAL_INTERPOLATION_SMOOTH);
    EXPECT_TRUE(targetStateChanged);
    EXPECT_TRUE(physicalStateChanging);
    targetStateChanged = false;

    glm::quat rotation = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(initialRotation.x),
            glm::radians(initialRotation.y),
            glm::radians(initialRotation.z)));
    const glm::vec3 gravity(0.f, 9.81f, 0.f);
    glm::vec3 velocity(0.f);
    glm::vec3 position(initialPosition.x, initialPosition.y, initialPosition.z);

    const float stepSeconds = android::physics::nsToSeconds(stepNs);
    const float maxConvergenceTimeNs =
            time + android::physics::secondsToNs(5.0f);
    long prevGyroMeasurementId = -1;
    long prevAccelMeasurementId = -1;
    time += stepNs / 2;
    int stepsRemainingAfterStable = 10;
    while (physicalStateChanging || stepsRemainingAfterStable > 0) {
        ASSERT_LT(time, maxConvergenceTimeNs)
                << "Physical state did not stabilize";
        if (!physicalStateChanging) {
          stepsRemainingAfterStable--;
        }
        if (time < 500000000 && time + stepNs >= 500000000) {
            physicalModel_setTargetRotation(
                    model, targetRotation, PHYSICAL_INTERPOLATION_SMOOTH);
            physicalModel_setTargetPosition(
                    model, targetPosition, PHYSICAL_INTERPOLATION_SMOOTH);
            EXPECT_TRUE(targetStateChanged);
            targetStateChanged = false;
        }
        physicalModel_setCurrentTime(model, time);
        long gyroMeasurementId;
        const vec3 measuredGyroscope = physicalModel_getGyroscope(
                model, &gyroMeasurementId);
        if (physicalStateChanging) {
            ASSERT_NE(prevGyroMeasurementId, gyroMeasurementId);
        }
        prevGyroMeasurementId = gyroMeasurementId;
        const glm::vec3 deviceSpaceRotationalVelocity(
                measuredGyroscope.x,
                measuredGyroscope.y,
                measuredGyroscope.z);
        const glm::vec3 rotationalVelocity =
                rotation * deviceSpaceRotationalVelocity;
        const glm::mat4 deltaRotationMatrix = glm::eulerAngleXYZ(
                rotationalVelocity.x * stepSeconds,
                rotationalVelocity.y * stepSeconds,
                rotationalVelocity.z * stepSeconds);

        rotation = glm::quat_cast(deltaRotationMatrix) * rotation;

        long accelMeasurementId;
        const vec3 measuredAcceleration = physicalModel_getAccelerometer(
                model, &accelMeasurementId);
        if (physicalStateChanging) {
            ASSERT_NE(prevAccelMeasurementId, accelMeasurementId);
        }
        prevAccelMeasurementId = accelMeasurementId;
        const glm::vec3 acceleration(measuredAcceleration.x,
                               measuredAcceleration.y,
                               measuredAcceleration.z);
        velocity += (rotation * acceleration - gravity) *
                stepSeconds;
        position += velocity * stepSeconds;

        time += stepNs;
    }

    const glm::quat targetRotationQuat = glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(targetRotation.x),
            glm::radians(targetRotation.y),
            glm::radians(targetRotation.z)));

    EXPECT_NEAR(targetRotationQuat.x, rotation.x, 0.001f);
    EXPECT_NEAR(targetRotationQuat.y, rotation.y, 0.001f);
    EXPECT_NEAR(targetRotationQuat.z, rotation.z, 0.001f);
    EXPECT_NEAR(targetRotationQuat.w, rotation.w, 0.001f);

    const vec3 integratedPosition {position.x, position.y, position.z};

    EXPECT_VEC3_NEAR(targetPosition, integratedPosition, 0.01f);

    EXPECT_FALSE(targetStateChanged);
    physicalModel_setPhysicalStateAgent(model, nullptr);

    physicalModel_free(model);
}

TEST(PhysicalModel, FoldableInitialize) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    android_hw->hw_lcd_width = 1260;
    android_hw->hw_lcd_height = 2400;
    android_hw->hw_sensor_hinge = true;
    android_hw->hw_sensor_hinge_count = 2;
    android_hw->hw_sensor_hinge_type = 0;
    android_hw->hw_sensor_hinge_sub_type = 1;
    android_hw->hw_sensor_hinge_ranges = (char*)"0- 360, 0-180";
    android_hw->hw_sensor_hinge_defaults = (char*)"180,90";
    android_hw->hw_sensor_hinge_areas = (char*)"25-10, 50-10";
    android_hw->hw_sensor_posture_list = (char*)"1, 2,3 ,  4";
    android_hw->hw_sensor_hinge_angles_posture_definitions = (char*)"0-30&0-15,  30-150 & 15-75,150-330&75-165, 330-360&165-180";

    PhysicalModel* model = physicalModel_new();
    physicalModel_setCurrentTime(model, 1000000000L);

    struct FoldableState ret;
    physicalModel_getFoldableState(model, &ret);
    EXPECT_EQ(180, ret.currentHingeDegrees[0]);
    EXPECT_EQ(90, ret.currentHingeDegrees[1]);
    EXPECT_EQ(2, ret.config.numHinges);
    EXPECT_EQ(ANDROID_FOLDABLE_HORIZONTAL_SPLIT, ret.config.type);
    EXPECT_EQ(ANDROID_FOLDABLE_HINGE_HINGE, ret.config.hingesSubType);
    EXPECT_EQ(0, ret.config.hingeParams[0].displayId);
    EXPECT_EQ(0, ret.config.hingeParams[0].x);
    EXPECT_EQ(600, ret.config.hingeParams[0].y);
    EXPECT_EQ(1260, ret.config.hingeParams[0].width);
    EXPECT_EQ(10, ret.config.hingeParams[0].height);
    EXPECT_EQ(0, ret.config.hingeParams[0].minDegrees);
    EXPECT_EQ(360, ret.config.hingeParams[0].maxDegrees);
    EXPECT_EQ(0, ret.config.hingeParams[1].displayId);
    EXPECT_EQ(0, ret.config.hingeParams[1].x);
    EXPECT_EQ(1200, ret.config.hingeParams[1].y);
    EXPECT_EQ(1260, ret.config.hingeParams[1].width);
    EXPECT_EQ(10, ret.config.hingeParams[1].height);
    EXPECT_EQ(0, ret.config.hingeParams[1].minDegrees);
    EXPECT_EQ(180, ret.config.hingeParams[1].maxDegrees);
    EXPECT_EQ(180, ret.config.hingeParams[0].defaultDegrees);
    EXPECT_EQ(90, ret.config.hingeParams[1].defaultDegrees);
    EXPECT_EQ(3, ret.currentPosture);
    physicalModel_free(model);
}
