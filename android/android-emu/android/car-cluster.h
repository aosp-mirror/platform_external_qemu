/* Copyright (C) 2018 The Android Open Source Project
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

#include <stdlib.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER
typedef void (*car_cluster_callback_t)(const uint8_t*, int);

extern void set_car_cluster_call_back(car_cluster_callback_t callback);

extern void android_car_cluster_init(void);

extern void android_send_car_cluster_data(const uint8_t* msg, int msgLen);
ANDROID_END_HEADER
