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

#include "android/automation/AutomationController.h"
#include "android/emulation/control/sensors_agent.h"

#include "android/hw-sensors.h"

int physical_parameter_target_set(int parameterId,
                                  const float* val,
                                  const size_t count,
                                  int interpolation_method) {
    return android_physical_model_set(parameterId, val, count,
                                      interpolation_method);
}

int physical_parameter_get(int parameterId,
                           float* const* out,
                           const size_t count,
                           int parameter_value_type) {
    int result = android_physical_model_get(
            parameterId, out, count,
            static_cast<ParameterValueType>(parameter_value_type));
    return result;
}

int physical_parameter_get_size(int parameterId, size_t* size) {
    return android_physical_model_get_size(parameterId, size);
}

int coarse_orientation_set(int orientation) {
    return android_sensors_set_coarse_orientation(
            static_cast<AndroidCoarseOrientation>(orientation), 0.f);
}

int sensor_override_set(int sensorId, const float* val, const size_t count) {
    return android_sensors_override_set(sensorId, val, count);
}

int sensor_get(int sensorId, float* const* out, const size_t count) {
    return android_sensors_get(sensorId, out, count);
}

int sensor_get_size(int sensorId, size_t* size) {
    return android_sensors_get_size(sensorId, size);
}

int sensor_get_delay_ms() {
    return android_sensors_get_delay_ms();
}

int physical_state_agent_set(const struct QAndroidPhysicalStateAgent* agent) {
    return android_physical_agent_set(agent);
}

void automation_advance_time() {
    android::automation::AutomationController::tryAdvanceTime();
}

static const QAndroidSensorsAgent sQAndroidSensorsAgent = {
        .setPhysicalParameterTarget = physical_parameter_target_set,
        .getPhysicalParameter = physical_parameter_get,
        .getPhysicalParameterSize = physical_parameter_get_size,
        .setCoarseOrientation = coarse_orientation_set,
        .setSensorOverride = sensor_override_set,
        .getSensor = sensor_get,
        .getSensorSize = sensor_get_size,
        .getDelayMs = sensor_get_delay_ms,
        .setPhysicalStateAgent = physical_state_agent_set,
        .advanceTime = automation_advance_time};
extern "C" const QAndroidSensorsAgent* const gQAndroidSensorsAgent =
        &sQAndroidSensorsAgent;
