/* Copyright (C) 2006-2010 The Android Open Source Project
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

#ifndef QEMU_ANDROID_EMULATOR_WINDOW_H
#define QEMU_ANDROID_EMULATOR_WINDOW_H

#include "android/cmdline-option.h"
#include "android/framebuffer.h"
#include "android/skin/file.h"
#include "android/skin/keyboard.h"
#include "android/skin/window.h"
#include "android/utils/aconfig-file.h"

#include "android/skin/ui.h"

typedef struct {
    AConfig*       aconfig;
    SkinFile*      layout_file;
    int            win_x;
    int            win_y;
    SkinUI*        ui;
    SkinImage*     onion;
    SkinRotation   onion_rotation;
    int            onion_alpha;

    AndroidOptions  opts[1];  /* copy of options */
} EmulatorWindow;

/* Gets a pointer to a EmulatorWindow structure instance. */
EmulatorWindow*
emulator_window_get(void);

void
android_emulator_set_window_scale(double  scale, int  is_dpi);

/* Initializes EmulatorWindow structure instance. */
int
emulator_window_init(EmulatorWindow*  emulator,
                     AConfig*         aconfig,
                     const char*      basepath,
                     int              x,
                     int              y,
                     AndroidOptions*  opts);

/* Uninitializes EmulatorWindow structure instance on exit. */
void
emulator_window_done(EmulatorWindow* emulator);

/* Gets emulator's layout. */
SkinLayout*
emulator_window_get_layout(EmulatorWindow* emulator);

/* Gets framebuffer for the first display. */
QFrameBuffer*
emulator_window_get_first_framebuffer(EmulatorWindow* emulator);

#endif  // QEMU_ANDROID_EMULATOR_WINDOW_H
