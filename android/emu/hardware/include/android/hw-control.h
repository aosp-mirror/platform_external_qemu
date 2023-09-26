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
#include "android/emulation/control/hw_control_agent.h"
#include <stdint.h>


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