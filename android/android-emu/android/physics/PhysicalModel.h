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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

struct QAndroidPhysicalStateAgent;

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
struct PhysicalModel* _physicalModel_new();

/* Destroy and free a physical model */
void _physicalModel_free(struct PhysicalModel* model);

/* Set the target position toward which the physical model should move */
void _physicalModel_setTargetPosition(struct PhysicalModel* model,
        struct vec3 position, bool instantaneous);

/* Set the rotation toward which the modeled object should move */
void _physicalModel_setTargetRotation(struct PhysicalModel* model,
        struct vec3 rotation, bool instantaneous);

/* Set the strength of the ambient magnetic field */
void _physicalModel_setTargetMagneticField(struct PhysicalModel* model,
        struct vec3 field, bool instantaneous);

/* Set the ambient temperature */
void _physicalModel_setTargetTemperature(struct PhysicalModel* model,
        float celsius, bool instantaneous);

/* Set the proximity value */
void _physicalModel_setTargetProximity(struct PhysicalModel* model,
        float centimeters, bool instantaneous);

/* Set the ambient light value */
void _physicalModel_setTargetLight(struct PhysicalModel* model,
        float lux, bool instantaneous);

/* Set the ambient barometric pressure */
void _physicalModel_setTargetPressure(struct PhysicalModel* model,
        float hPa, bool instantaneous);

/* Set the ambient relative humidity */
void _physicalModel_setTargetHumidity(struct PhysicalModel* model,
        float percentage, bool instantaneous);

struct vec3 _physicalModel_getTargetPosition(struct PhysicalModel* model);
struct vec3 _physicalModel_getTargetRotation(struct PhysicalModel* model);
struct vec3 _physicalModel_getTargetMagneticField(struct PhysicalModel* model);
float _physicalModel_getTargetTemperature(struct PhysicalModel* model);
float _physicalModel_getTargetProximity(struct PhysicalModel* model);
float _physicalModel_getTargetLight(struct PhysicalModel* model);
float _physicalModel_getTargetPressure(struct PhysicalModel* model);
float _physicalModel_getTargetHumidity(struct PhysicalModel* model);

/* Overrides for all sensor values */
#define OVERRIDE_FUNCTION_NAME(x) _physicalModel_override##x
#define SENSOR_(x,y,z) void OVERRIDE_FUNCTION_NAME(y)(\
        struct PhysicalModel* model,\
        z override_value);
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME

/* Getters for all sensor values */
#define GET_FUNCTION_NAME(x) _physicalModel_get##x
#define SENSOR_(x,y,z) z GET_FUNCTION_NAME(y)(\
        struct PhysicalModel* model);
    PHYSICAL_SENSORS_LIST
#undef SENSOR_ 
#undef GET_FUNCTION_NAME

/* Sets the callbacks used to signal changing state */
void _physicalModel_setPhysicalStateAgent(struct PhysicalModel* model,
        const struct QAndroidPhysicalStateAgent* agent);

ANDROID_END_HEADER
