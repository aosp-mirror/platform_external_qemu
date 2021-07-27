/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include "android/utils/compiler.h"

#include "android/physics/physical_state_agent.h"

ANDROID_BEGIN_HEADER

typedef struct QAndroidSensorsAgent {
    // Sets the target values of a given physical parameter.
    // Input: |parameterId| determines which parameter's values to set, i.e.
    //                      PHYSICAL_PARAMETER_POSITION,
    //                      PHYSICAL_PARAMETER_ROTATITION,
    //                      etc.
    //        |interpolationMethod| determines which interpolation type to use,
    //                              i.e.
    //                              PHYSICAL_INTERPOLATION_SMOOTH,
    //                              PHYSICAL_INTERPOLATION_STEP
    int (*setPhysicalParameterTarget)(
            int parameterId, float a, float b, float c,
            int interpolationMethod);

    // Gets the target values of a given physical parameter.
    // Input: |parameterId| determines which parameter's values to retrieve.
    //        |parameterValueType| determines whether to retrieve the
    //                         parameter's target, or current/instantaneous
    //                         value, i.e.
    //                         PARAMETER_VALUE_TYPE_TARGET,
    //                         PARAMETER_VALUE_TYPE_CURRENT
    int (*getPhysicalParameter)(
            int parameterId, float *a, float *b, float *c,
            int parameterValueType);

    // Sets the coarse orientation of the modeled device.
    // Input: |orientation| determines which coarse orientation to use, i.e.
    //                      ANDROID_COARSE_PORTRAIT,
    //                      ANDROID_COARSE_LANDSCAPE,
    //                      etc.
    int (*setCoarseOrientation)(int orientation);

    // Sets the values of a given sensor to the specified override.
    // Input: |sensorId| determines which sensor's values to retrieve, i.e.
    //                   ANDROID_SENSOR_ACCELERATION,
    //                   ANDROID_SENSOR_GYROSCOPE,
    //                   etc.
    int (*setSensorOverride)(int sensorId, float a, float b, float c);

    // Reads the values from a given sensor.
    // Input: |sensorId| determines which sensor's values to retrieve, i.e.
    //                   ANDROID_SENSOR_ACCELERATION,
    //                   ANDROID_SENSOR_GYROSCOPE,
    //                   etc.
    int (*getSensor)(int sensorId, float *a, float *b, float *c);

    // Gets the current sensor update delay in milliseconds.
    int (*getDelayMs)();

    // Sets the agent used to receive callbacks to used to track whether the
    // state of the physical model is currently changing.
    int (*setPhysicalStateAgent)(const struct QAndroidPhysicalStateAgent* agent);

    // Advance the current time on the sensors agent.
    void (*advanceTime)();
} QAndroidSensorsAgent;

ANDROID_END_HEADER
