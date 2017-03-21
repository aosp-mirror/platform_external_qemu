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

#include "android/emulation/control/sensors_agent.h"

#include "android/hw-sensors.h"

#include <stdio.h>

int sensor_set(int sensorId, float a, float b, float c) {
    return android_sensors_set(sensorId, a, b, c);
}

int sensor_get(int sensorId, float *a, float *b, float *c) {
    return android_sensors_get(sensorId, a, b, c);
}

static const QAndroidSensorsAgent sQAndroidSensorsAgent = {
    .setSensor = sensor_set,
    .getSensor = sensor_get};
const QAndroidSensorsAgent* const gQAndroidSensorsAgent =
    &sQAndroidSensorsAgent;
