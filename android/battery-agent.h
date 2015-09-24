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

ANDROID_BEGIN_HEADER

#include "stdbool.h"


enum BatteryHealth { BATTERY_HEALTH_GOOD, BATTERY_HEALTH_FAILED, BATTERY_HEALTH_DEAD,
                     BATTERY_HEALTH_OVERVOLTAGE, BATTERY_HEALTH_OVERHEATED,
                     BATTERY_HEALTH_UNKNOWN,
                     BATTERY_HEALTH_NUM_ENTRIES };

enum BatteryStatus { BATTERY_STATUS_UNKNOWN, BATTERY_STATUS_CHARGING,
                     BATTERY_STATUS_DISCHARGING, BATTERY_STATUS_NOT_CHARGING,
                     BATTERY_STATUS_FULL,
                     BATTERY_STATUS_NUM_ENTRIES };

typedef struct BatteryAgent {
    // Sets whether the battery is charging or not
    void (*setIsCharging)(bool isCharging);

    // Sets the current charge level
    // Input: |percentFull| integer percentage, 0 .. 100
    void (*setChargeLevel)(int percentFull);

    // Sets the battery health
    // Input: |health| one of the BatteryHealth enum values, above
    void (*setHealth)(enum BatteryHealth health);

    // Sets the battery status
    // Input: |status| one of the BatteryStatus enum values, above
    void (*setStatus)(enum BatteryStatus status);
} BatteryAgent;

ANDROID_END_HEADER
