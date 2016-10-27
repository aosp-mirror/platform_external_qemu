/* Copyright 2016 The Android Open Source Project
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
#include <stdint.h>

#pragma once

/* initialize */
extern void android_hw_nfc_init(void);

/* Send a hexadecimal string message to the emulator nfc. */
void android_hw_nfc_send_message(const void* data, size_t msg_len);
