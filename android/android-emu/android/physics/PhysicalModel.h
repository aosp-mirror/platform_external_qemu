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

#pragma once

#include "android/physics/Physics.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* forward declarations */
struct QAndroidPhysicalStateAgent;
struct Stream;

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3;

#define PHYSICAL_SENSORS_LIST  \
    SENSOR_(ACCELERATION,Accelerometer,vec3) \
    SENSOR_(GYROSCOPE,Gyroscope,vec3) \
    SENSOR_(MAGNETOMETER,Magnetometer,vec3) \
    SENSOR_(ORIENTATION,Orientation,vec3) \
    SENSOR_(TEMPERATURE,Temperature,float) \
    SENSOR_(PROXIMITY,Proximity,float) \
    SENSOR_(LIGHT,Light,float) \
    SENSOR_(PRESSURE,Pressure,float) \
    SENSOR_(HUMIDITY,Humidity,float) \
    SENSOR_(MAGNETOMETER_UNCALIBRATED,MagnetometerUncalibrated,vec3) \
    SENSOR_(GYROSCOPE_UNCALIBRATED,GyroscopeUncalibrated,vec3) \

typedef enum {
#define PHYSICAL_SENSOR_NAME(x) PHYSICAL_SENSOR_##x
#define  SENSOR_(x,y,z) PHYSICAL_SENSOR_NAME(x),
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_SENSOR_NAME
    PHYSICAL_SENSORS_MAX /* do not remove */
} PhysicalSensor;

/*
 * Implements a model of an ambient environment containing a rigid
 * body, and produces accurately simulated sensor values for various
 * sensors in this environment.
 *
 * The physical model should be updated with target ambient and rigid
 * body state, and regularly polled for the most recent sensor values.
 *
 * Components that only require updates when the model is actively
 * changing (i.e. not at rest) should register state change callbacks
 * via setStateChangingCallbacks.  TargetStateChange callbacks occur
 * on the same thread that setTargetXXXXXX is called from.  Sensor
 * state changing callbacks may occur on an arbitrary thread.
 */

typedef struct PhysicalModel {
    /* Opaque pointer used by the physical model c api. */
    void* opaque;
} PhysicalModel;

/* Allocate and initialize a physical model */
PhysicalModel* physicalModel_new();

/* Destroy and free a physical model */
void physicalModel_free(PhysicalModel* model);

/* Set the target position toward which the physical model should move */
void physicalModel_setTargetPosition(PhysicalModel* model,
        struct vec3 position, PhysicalInterpolation mode);

/* Set the rotation toward which the modeled object should move */
void physicalModel_setTargetRotation(PhysicalModel* model,
        struct vec3 rotation, PhysicalInterpolation mode);

/* Set the strength of the ambient magnetic field */
void physicalModel_setTargetMagneticField(PhysicalModel* model,
        struct vec3 field, PhysicalInterpolation mode);

/* Set the ambient temperature */
void physicalModel_setTargetTemperature(PhysicalModel* model,
        float celsius, PhysicalInterpolation mode);

/* Set the proximity value */
void physicalModel_setTargetProximity(PhysicalModel* model,
        float centimeters, PhysicalInterpolation mode);

/* Set the ambient light value */
void physicalModel_setTargetLight(PhysicalModel* model,
        float lux, PhysicalInterpolation mode);

/* Set the ambient barometric pressure */
void physicalModel_setTargetPressure(PhysicalModel* model,
        float hPa, PhysicalInterpolation mode);

/* Set the ambient relative humidity */
void physicalModel_setTargetHumidity(PhysicalModel* model,
        float percentage, PhysicalInterpolation mode);

struct vec3 physicalModel_getTargetPosition(PhysicalModel* model);
struct vec3 physicalModel_getTargetRotation(PhysicalModel* model);
struct vec3 physicalModel_getTargetMagneticField(PhysicalModel* model);
float physicalModel_getTargetTemperature(PhysicalModel* model);
float physicalModel_getTargetProximity(PhysicalModel* model);
float physicalModel_getTargetLight(PhysicalModel* model);
float physicalModel_getTargetPressure(PhysicalModel* model);
float physicalModel_getTargetHumidity(PhysicalModel* model);

/* Overrides for all sensor values */
#define OVERRIDE_FUNCTION_NAME(x) physicalModel_override##x
#define SENSOR_(x,y,z) void OVERRIDE_FUNCTION_NAME(y)(\
        PhysicalModel* model,\
        z override_value);
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME

/*
 * Get the current sensor value, along with a measurement id that can be
 * compared with previous measurement ids for the same sensor in order to
 * determine whether the value has been updated since a previous call to get
 * sensor data.
 */
#define GET_FUNCTION_NAME(x) physicalModel_get##x
#define SENSOR_(x,y,z) z GET_FUNCTION_NAME(y)(\
        PhysicalModel* model, long* measurement_id);
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef GET_FUNCTION_NAME

/* Sets or unsets the callbacks used to signal changing state */
void physicalModel_setPhysicalStateAgent(PhysicalModel* model,
        const struct QAndroidPhysicalStateAgent* agent);

/* Save the physical model state to the specified stream. */
void physicalModel_save(PhysicalModel* model, Stream* f);

/* Load the physical model state from the specified stream. */
int physicalModel_load(PhysicalModel* model, Stream* f);

ANDROID_END_HEADER
