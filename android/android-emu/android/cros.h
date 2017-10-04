/* Copyright (C) 2017 The Android Open Source Project
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

#include "android/emulation/serial_line.h"

/* this is the internal character driver used to communicate with the
 * Chrome OS guest. see qemu_chr_open() in vl.c */
extern CSerialLine* cros_serial_line;

// Send a message to Chrome OS guest.
extern void cros_send_message(const char* message);
