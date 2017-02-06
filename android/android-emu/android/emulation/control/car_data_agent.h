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

#include "android/utils/compiler.h"
#ifdef __cplusplus
#include <functional>
#endif

ANDROID_BEGIN_HEADER

typedef struct QCarDataAgent {
  #ifdef __cplusplus
       void (*setCallback)(const std::function<void(const char*, int)>& callback);
        #endif

    void (*sendCarData)(const char* msg, int len);
} QCarDataAgent;

ANDROID_END_HEADER