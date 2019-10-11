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

#include "android/base/async/ThreadLooper.h"
#include "android/base/system/System.h"
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/VmLock.h"
#include "android/utils/looper.h"

#include <functional>
#include <stdio.h>

using android::base::System;
using android::base::ThreadLooper;

class BatteryAccessor {
public:
    using Call = std::function<int()>;
    BatteryAccessor(int* returnPtr, Call func) :
        mReturnPtr(returnPtr),
        mCall(func) { run(mReturnPtr); }

    void run(int* out) {
        if (!android_getMainLooper()) {
            fprintf(stderr, "%s: no main looper yet\n", __func__);
            if (out) *out = mCall();
            return;
        }

        bool done = false;
        fprintf(stderr, "BatteryAccessor::%s: run something. out? %p\n", __func__, out);
        ThreadLooper::runOnMainLooper(
            [this, out, &done] {
            fprintf(stderr, "BatteryAccessor::%s: running.\n", __func__);
            if (out) *out = mCall();
            done = true;
        });
        while (!done) System::get()->sleepMs(50);
        fprintf(stderr, "BatteryAccessor::%s: done.\n", __func__);
    }

private:
    int* mReturnPtr;
    Call mCall;
};

extern "C" {

#include "qemu/typedefs.h"
#include "hw/misc/goldfish_battery.h"

static void battery_setHasBattery(bool hasBattery) {
    BatteryAccessor accessor(0, [hasBattery] {
        goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HAS_BATTERY, hasBattery);
        return 0;
    });
}

static bool battery_hasBattery() {
    int has = 0;
    BatteryAccessor accessor(&has, [] {
        return goldfish_battery_read_prop(POWER_SUPPLY_PROP_HAS_BATTERY);
    });
    return (bool)has;
}

static void battery_setIsBatteryPresent(bool isPresent) {
    BatteryAccessor accessor(0, [isPresent] {
        goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_PRESENT, isPresent);
        return 0;
    });
}

static bool battery_present() {
    int res = 0;
    BatteryAccessor accessor(&res, [] {
        return goldfish_battery_read_prop(POWER_SUPPLY_PROP_PRESENT);
    });
    return (bool)res;
}

static void battery_setIsCharging(bool isCharging) {
    BatteryAccessor accessor(0, [isCharging] {
        goldfish_battery_set_prop(1, POWER_SUPPLY_PROP_ONLINE, isCharging);
        return 0;
    });
}

static bool battery_charging() {
    int res = 0;
    BatteryAccessor accessor(&res, [] {
        return (bool)goldfish_battery_read_prop(POWER_SUPPLY_PROP_ONLINE);
    });
    return (bool)res;
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
    BatteryAccessor accessor(0, [percentFull] {
        goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, percentFull);
        return 0;
    });
}

static int battery_chargeLevel() {
    int res = 0;
    BatteryAccessor accessor(&res, [] {
        return goldfish_battery_read_prop(POWER_SUPPLY_PROP_CAPACITY);
    });
    return res;
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

    BatteryAccessor accessor(0, [value] {
        goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_HEALTH, value);
        return 0;
    });
}

static enum BatteryHealth battery_health() {
    int res = 0;
    BatteryAccessor accessor(&res, [] {
        switch ( goldfish_battery_read_prop(POWER_SUPPLY_PROP_HEALTH) ) {
            case POWER_SUPPLY_HEALTH_GOOD:           return BATTERY_HEALTH_GOOD;
            case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE: return BATTERY_HEALTH_FAILED;
            case POWER_SUPPLY_HEALTH_DEAD:           return BATTERY_HEALTH_DEAD;
            case POWER_SUPPLY_HEALTH_OVERVOLTAGE:    return BATTERY_HEALTH_OVERVOLTAGE;
            case POWER_SUPPLY_HEALTH_OVERHEAT:       return BATTERY_HEALTH_OVERHEATED;
            case POWER_SUPPLY_HEALTH_UNKNOWN:        return BATTERY_HEALTH_UNKNOWN;

            default:                                 return BATTERY_HEALTH_UNKNOWN;
        }
    });
    return (BatteryHealth)res;
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

    BatteryAccessor accessor(0, [value] {
        goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_STATUS, value);
        return 0;
    });
}

static enum BatteryStatus battery_status() {
    int res = 0;
    BatteryAccessor accessor(&res, [] {
        switch ( goldfish_battery_read_prop(POWER_SUPPLY_PROP_STATUS) ) {
            case POWER_SUPPLY_STATUS_UNKNOWN:       return BATTERY_STATUS_UNKNOWN;
            case POWER_SUPPLY_STATUS_CHARGING:      return BATTERY_STATUS_CHARGING;
            case POWER_SUPPLY_STATUS_DISCHARGING:   return BATTERY_STATUS_DISCHARGING;
            case POWER_SUPPLY_STATUS_NOT_CHARGING:  return BATTERY_STATUS_NOT_CHARGING;
            case POWER_SUPPLY_STATUS_FULL:          return BATTERY_STATUS_FULL;

            default:                                return BATTERY_STATUS_UNKNOWN;
        }
    });
    return (BatteryStatus)res;
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
