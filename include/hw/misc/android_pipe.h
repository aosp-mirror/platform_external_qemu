/* Copyright (C) 2011 The Android Open Source Project
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
#ifndef _HW_ANDROID_PIPE_H
#define _HW_ANDROID_PIPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hw/hw.h"

extern bool qemu2_adb_server_init(int port);

#include "android/emulation/android_pipe_device.h"
#include "android/opengles-pipe.h"

extern void android_zero_pipe_init(void);
extern void android_pingpong_init(void);
extern void android_throttle_init(void);
extern void android_adb_dbg_backend_init(void);
extern void android_adb_backend_init(void);
extern void android_sensors_init(void);

#endif /* _HW_ANDROID_PIPE_H */
