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

#ifndef ANDROID_BATTERY_AGENT_H
#define ANDROID_BATTERY_AGENT_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

enum BatteryHealth { Battery_H_Good, Battery_H_Failed, Battery_H_Dead,
                     Battery_H_Overvoltage, Battery_H_Overheated, Battery_H_Unknown,
                     Battery_H_NUM_ENTRIES };

enum BatteryStatus { Battery_S_Unknown, Battery_S_Charging,
                     Battery_S_Discharging, Battery_S_Not_Charging,
                     Battery_S_Full,
                     Battery_S_NUM_ENTRIES };

typedef struct BatteryAgent {
    // Sets whether the battery is charging or not
    void (*setIsCharging)(int isCharging);

    // Sets the current charge level
    // Input: 0 .. 100
    void (*setChargeLevel)(int percentFull);

    // Sets the battery health
    // Input: enum BatteryHealth, above
    void (*setHealth)(enum BatteryHealth);

    // Sets the battery status
    // Input: enum BatteryStatus, above
    void (*setStatus)(enum BatteryStatus);
} BatteryAgent;

ANDROID_END_HEADER

#endif // ANDROID_BATTERY_AGENT_H
