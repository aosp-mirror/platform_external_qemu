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

#include "android/qemu-control-impl.h"

#include "android/emulation/control/battery_agent.h"
#include "hw/power_supply.h"
#include "hw/android/goldfish/device.h"

static void battery_setIsBatteryPresent(bool isPresent) {
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_PRESENT, isPresent);
}

static void battery_setIsCharging(bool isCharging) {
    goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, isCharging);
}

static void battery_setChargeLevel(int percentFull) {
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, percentFull);
}

static void battery_setHealth(enum BatteryHealth health) {
    int value;

    switch (health) {
        case BATTERY_HEALTH_GOOD:          value = POWER_SUPPLY_HEALTH_GOOD;            break;
        case BATTERY_HEALTH_FAILED:        value = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;  break;
        case BATTERY_HEALTH_DEAD:          value = POWER_SUPPLY_HEALTH_DEAD;            break;
        case BATTERY_HEALTH_OVERVOLTAGE:   value = POWER_SUPPLY_HEALTH_OVERVOLTAGE;     break;
        case BATTERY_HEALTH_OVERHEATED:    value = POWER_SUPPLY_HEALTH_OVERHEAT;        break;
        case BATTERY_HEALTH_UNKNOWN:       value = POWER_SUPPLY_HEALTH_UNKNOWN;         break;

        default:                           value = POWER_SUPPLY_HEALTH_UNKNOWN;         break;
    }

    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH, value);
}

static void battery_setStatus(enum BatteryStatus status) {
    int value;

    switch (status) {
        case BATTERY_STATUS_UNKNOWN:       value = POWER_SUPPLY_STATUS_UNKNOWN;       break;
        case BATTERY_STATUS_CHARGING:      value = POWER_SUPPLY_STATUS_CHARGING;      break;
        case BATTERY_STATUS_DISCHARGING:   value = POWER_SUPPLY_STATUS_DISCHARGING;   break;
        case BATTERY_STATUS_NOT_CHARGING:  value = POWER_SUPPLY_STATUS_NOT_CHARGING;  break;
        case BATTERY_STATUS_FULL:          value = POWER_SUPPLY_STATUS_FULL;          break;

        default:                           value = POWER_SUPPLY_STATUS_UNKNOWN;       break;
    }

    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS, value);
}

static const QAndroidBatteryAgent sQAndroidBatteryAgent = {
        .setIsBatteryPresent = battery_setIsBatteryPresent,
        .setIsCharging = battery_setIsCharging,
        .setChargeLevel = battery_setChargeLevel,
        .setHealth = battery_setHealth,
        .setStatus = battery_setStatus,
        .batteryDisplay = goldfish_battery_display};
const QAndroidBatteryAgent* const gQAndroidBatteryAgent =
        &sQAndroidBatteryAgent;
