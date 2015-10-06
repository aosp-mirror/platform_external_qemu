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

#include "android/sensors-agent.h"
#include "android/sensors-agent-impl.h"
#include "android/hw-sensors.h"

int sensor_set(int sensorId, float a, float b, float c) {
    return android_sensors_set(sensorId, a, b, c);
}

int sensor_get(int sensorId, float *a, float *b, float *c) {
    return android_sensors_get(sensorId, a, b, c);
}

