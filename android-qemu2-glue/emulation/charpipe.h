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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* Forward-declare it as qemu-common.h doesn't like to be included here */
typedef struct Chardev Chardev;
typedef struct CharBackend CharBackend;

/* open two connected character drivers that can be used to communicate by internal
 * QEMU components. For Android, this is used to connect an emulated serial port
 * with the android modem
 */
extern int qemu_chr_open_charpipe(Chardev* *pfirst,
                                  Chardev* *psecond);

/* create a buffering character driver for a given endpoint. The result will buffer
 * anything that is sent to it but cannot be sent to the endpoint immediately.
 * On the other hand, if the endpoint calls can_read() or read(), these calls
 * are passed immediately to the can_read() or read() handlers of the result.
 */
extern Chardev *qemu_chr_open_buffer(Chardev*  endpoint);

/* must be called from the main event loop to poll all charpipes */
extern void qemu_charpipe_poll(void);

ANDROID_END_HEADER
