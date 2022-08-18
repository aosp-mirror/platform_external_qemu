// Copyright (C) 2007-2008 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#pragma once

#include "android/utils/compiler.h"

#include <stdint.h>

// a callback function called when the system wants to change the brightness
// of a given light. 'light' is a string which can be one of:
// 'lcd_backlight', 'button_backlight' or 'Keyboard_backlight'
//
// brightness is an integer (acceptable range are 0..255), however the
// default is around 105, and we probably don't want to dim the emulator's
// output at that level.
//
typedef void (*AndroidHwLightBrightnessFunc)(void* opaque,
                                             const char* light,
                                             int brightness);

// used to record a hw control 'client'
typedef struct {
    AndroidHwLightBrightnessFunc light_brightness;
} AndroidHwControlFuncs;

ANDROID_BEGIN_HEADER

// used to initialize the hardware control support
extern void android_hw_control_init(void);

// used to register a new hw-control back-end
extern void android_hw_control_set(void* opaque,
                                   const AndroidHwControlFuncs* funcs);

extern void android_hw_control_set_brightness(const char* light_name,
                                              uint32_t value);

extern uint32_t android_hw_control_get_brightness(const char* light_name);

ANDROID_END_HEADER