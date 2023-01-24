/* Copyright (C) 2022 The Android Open Source Project
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
#include "android/car-physics.h"

#include "android/console.h"
#include "android/hw-sensors.h"
#include "android/physics/PhysicalModel.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <string.h>

// forward declaration
typedef struct SkinLayout SkinLayout;

void android_car_physics_init() {
    // Phones and cars use a different default orientation.
    // See
    // https://source.android.com/docs/core/interaction/sensors/sensor-types#limited_axes_imu_sensors
    physicalModel_setGravity(android_physical_model_instance(), 0.f, 0.f,
                             -9.81f);

    // Phones are tilted by 4.75f in
    // external/qemu/android/android-emu/android/emulator-window.c
    // emulator_window_setup(...). Override this with zero.
    android_sensors_set_coarse_orientation(ANDROID_COARSE_PORTRAIT,
                                           0.f /* no tilt */);
}