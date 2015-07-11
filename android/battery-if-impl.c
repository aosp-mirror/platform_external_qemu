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

#include "battery-if.h"
#include "battery-if-impl.h"
#include "hw/power_supply.h"
#include "hw/android/goldfish/device.h"

void battery_setIsCharging(int isCharging) {
    goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, isCharging);
}

void battery_setChargeLevel(int percentFull) {
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, percentFull);
}

void battery_setHealth(enum BatteryHealth health) {
    int value = POWER_SUPPLY_HEALTH_UNKNOWN;

    switch (health) {
        case Battery_H_Good:          value = POWER_SUPPLY_HEALTH_GOOD;            break;
        case Battery_H_Failed:        value = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;  break;
        case Battery_H_Dead:          value = POWER_SUPPLY_HEALTH_DEAD;            break;
        case Battery_H_Overvoltage:   value = POWER_SUPPLY_HEALTH_OVERVOLTAGE;     break;
        case Battery_H_Overheated:    value = POWER_SUPPLY_HEALTH_OVERHEAT;        break;
        case Battery_H_Unknown:       value = POWER_SUPPLY_HEALTH_UNKNOWN;         break;
    }

    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH, value);
}

void battery_setStatus(enum BatteryStatus status) {
    printf("=====battery-if.c: battery_setStatus() not implemented\n");
}
