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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/VmLock.h"

#include <stdio.h>

extern "C" {

#include "qemu/typedefs.h"
#include "hw/misc/goldfish_battery.h"

static void battery_setHasBattery(bool hasBattery) {
    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HAS_BATTERY, hasBattery);
}

static bool battery_hasBattery() {
    android::RecursiveScopedVmLockIfInstance lock;
    return (bool)goldfish_battery_read_prop(POWER_SUPPLY_PROP_HAS_BATTERY);
}

static void battery_setIsBatteryPresent(bool isPresent) {
    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_PRESENT, isPresent);
}

static bool battery_present() {
    android::RecursiveScopedVmLockIfInstance lock;
    return (bool)goldfish_battery_read_prop(POWER_SUPPLY_PROP_PRESENT);
}

static void battery_setIsCharging(bool isCharging) {
    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, isCharging);
}

static bool battery_charging() {
    android::RecursiveScopedVmLockIfInstance lock;
    return (bool)goldfish_battery_read_prop(POWER_SUPPLY_PROP_ONLINE);
}

static void battery_setCharger(enum BatteryCharger charger) {
    battery_setIsCharging( charger != BATTERY_CHARGER_NONE );

    // TODO: Need to save the full enum and convey it to the AVD
}

static enum BatteryCharger battery_charger() {
    return ( battery_charging() ? BATTERY_CHARGER_AC : BATTERY_CHARGER_NONE );
    // TODO: Need to get the full enum from the AVD
}

static void battery_setChargeLevel(int percentFull) {
    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, percentFull);
}

static int battery_chargeLevel() {
    android::RecursiveScopedVmLockIfInstance lock;
    return goldfish_battery_read_prop(POWER_SUPPLY_PROP_CAPACITY);
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

    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH, value);
}

static enum BatteryHealth battery_health() {
    android::RecursiveScopedVmLockIfInstance lock;
    switch ( goldfish_battery_read_prop(POWER_SUPPLY_PROP_HEALTH) ) {
        case POWER_SUPPLY_HEALTH_GOOD:           return BATTERY_HEALTH_GOOD;
        case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE: return BATTERY_HEALTH_FAILED;
        case POWER_SUPPLY_HEALTH_DEAD:           return BATTERY_HEALTH_DEAD;
        case POWER_SUPPLY_HEALTH_OVERVOLTAGE:    return BATTERY_HEALTH_OVERVOLTAGE;
        case POWER_SUPPLY_HEALTH_OVERHEAT:       return BATTERY_HEALTH_OVERHEATED;
        case POWER_SUPPLY_HEALTH_UNKNOWN:        return BATTERY_HEALTH_UNKNOWN;

        default:                                 return BATTERY_HEALTH_UNKNOWN;
    }
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

    android::RecursiveScopedVmLockIfInstance lock;
    goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS, value);
}

static enum BatteryStatus battery_status() {
    android::RecursiveScopedVmLockIfInstance lock;
    switch ( goldfish_battery_read_prop(POWER_SUPPLY_PROP_STATUS) ) {
        case POWER_SUPPLY_STATUS_UNKNOWN:       return BATTERY_STATUS_UNKNOWN;
        case POWER_SUPPLY_STATUS_CHARGING:      return BATTERY_STATUS_CHARGING;
        case POWER_SUPPLY_STATUS_DISCHARGING:   return BATTERY_STATUS_DISCHARGING;
        case POWER_SUPPLY_STATUS_NOT_CHARGING:  return BATTERY_STATUS_NOT_CHARGING;
        case POWER_SUPPLY_STATUS_FULL:          return BATTERY_STATUS_FULL;

        default:                                return BATTERY_STATUS_UNKNOWN;
    }
}

typedef void (*BatteryDisplayCb)(void* opaque, LineConsumerCallback outputCallback);

static const QAndroidBatteryAgent sQAndroidBatteryAgent = {
        .setHasBattery = battery_setHasBattery,
        .hasBattery = battery_hasBattery,
        .setIsBatteryPresent = battery_setIsBatteryPresent,
        .present = battery_present,
        .setIsCharging = battery_setIsCharging,
        .charging = battery_charging,
        .setCharger = battery_setCharger,
        .charger = battery_charger,
        .setChargeLevel = battery_setChargeLevel,
        .chargeLevel = battery_chargeLevel,
        .setHealth = battery_setHealth,
        .health = battery_health,
        .setStatus = battery_setStatus,
        .status = battery_status,
        .batteryDisplay = (BatteryDisplayCb)goldfish_battery_display_cb
};
const QAndroidBatteryAgent* const gQAndroidBatteryAgent =
        &sQAndroidBatteryAgent;

} // extern "C"
