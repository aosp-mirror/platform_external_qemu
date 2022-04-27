/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/console.h"
#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/base/export.h"
#include "android/constants.h"
#include "android/user-config.h"
#include "android/utils/compiler.h"
#include "android/console.h"

ANDROID_BEGIN_HEADER

/** PLEASE DO NOT ANY MORE GLOBALS, INSTEAD INJECT THEM FROM 
 *  The agents are defined in console.h 
 */

/* this indicates that guest has mounted data partition */
extern int guest_data_partition_mounted;

/* this indicates that guest has boot completed */
extern int guest_boot_completed;

extern int arm_snapshot_save_completed;

extern int host_emulator_is_headless;

/* are we using the emulator in the android mode or plain qemu? */
extern int android_qemu_mode;

/* are we using android-emu libraries for a minimal configuration? */
extern int min_config_qemu_mode;

/* is android-emu running Fuchsia? */
extern int is_fuchsia;

extern int android_snapshot_update_timer;

extern AEMU_EXPORT AUserConfig* aemu_get_userConfigPtr();

ANDROID_END_HEADER
