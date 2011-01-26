/* Copyright (C) 2010 The Android Open Source Project
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

#ifndef _ANDROID_PROTOCOL_UI_COMMANDS_H
#define _ANDROID_PROTOCOL_UI_COMMANDS_H

/*
 * Contains declarations for UI control protocol used by the Core.
 * In essence, here are defined UI control commands sent by the Core to the UI.
 */

#include "android/protocol/ui-common.h"

/* Sets window scale. */
#define AUICMD_SET_WINDOWS_SCALE    1

/* Formats AUICMD_SET_WINDOWS_SCALE UI control command parameters.
 * Contains parameters required by android_emulator_set_window_scale routine.
 */
typedef struct UICmdSetWindowsScale {
    double  scale;
    int     is_dpi;
} UICmdSetWindowsScale;

#endif /* _ANDROID_PROTOCOL_UI_COMMANDS_H */
