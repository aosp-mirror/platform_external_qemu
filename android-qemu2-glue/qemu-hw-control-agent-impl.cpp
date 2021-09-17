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

#include "android/emulation/control/hw_control_agent.h"

#include "android/hw-control.h"

namespace {

void set_brightness(const char* light_name, uint32_t brightness) {
    android_hw_control_set_brightness(light_name, brightness);
}

uint32_t get_brightness(const char* light_name) {
    return android_hw_control_get_brightness(light_name);
}

const QAndroidHwControlAgent sQAndroidHwControlAgent = {
        .setBrightness = set_brightness,
        .getBrightness = get_brightness,
};

}  // namespace

extern "C" const QAndroidHwControlAgent* const gQAndroidHwControlAgent =
        &sQAndroidHwControlAgent;
