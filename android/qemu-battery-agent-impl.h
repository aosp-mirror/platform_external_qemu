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

#include "android/emulation/battery_agent.h"

ANDROID_BEGIN_HEADER

// Main entry point for populating a new QAndroidVMOperations for QEMU1.
// Caller takes ownership of returned object.
// See android/emulation/batter_agent.h to learn how to use this.
// Multiple calls return the same global instance.
const QAndroidBatteryAgent* qemu_get_android_battery_agent(void);

ANDROID_END_HEADER
