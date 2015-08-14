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

#ifndef ANDROID_BATTERY_AGENT_IMPL_H
#define ANDROID_BATTERY_AGENT_IMPL_H

#include "android/battery-agent.h"

void battery_setIsCharging(bool isCharging);
void battery_setChargeLevel(int percentFull);
void battery_setHealth(enum BatteryHealth health);
void battery_setStatus(enum BatteryStatus status);

#endif // ANDROID_BATTERY_AGENT_IMPL_H
