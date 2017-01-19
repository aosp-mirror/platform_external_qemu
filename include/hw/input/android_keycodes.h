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

/* This extends the list of key codes that are defined in linux_keycodes.h
*/

#pragma once

// Note: qemu-user-event-agent-impl.c limits key values to 0x3ff (1023)

#define ANDROID_KEY_APPSWITCH    580

// Keycodes added to android.view.KeyEvent in API 24, to support hardware
// buttons on Android Wear devices
#define ANDROID_KEY_STEM_PRIMARY 264
#define ANDROID_KEY_STEM_1 265
#define ANDROID_KEY_STEM_2 266
#define ANDROID_KEY_STEM_3 267
