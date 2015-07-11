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

#ifndef BATTERY_IF_H
#define BATTERY_IF_H

enum BatteryHealth { Battery_H_Good, Battery_H_Failed, Battery_H_Dead,
                     Battery_H_Overvoltage, Battery_H_Overheated, Battery_H_Unknown };

enum BatteryStatus { Battery_S_Good, Battery_S_Unknown };

typedef struct BatteryIf_s {
    void (*setIsCharging)(int isCharging);
    void (*setChargeLevel)(int percentFull);
    void (*setHealth)(enum BatteryHealth);
    void (*setStatus)(enum BatteryStatus);
} BatteryIf;


#endif // BATTERY_IF_H
