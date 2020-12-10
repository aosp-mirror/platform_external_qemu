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

#pragma once
#include "android/utils/compiler.h"
#include "android/utils/system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

ANDROID_BEGIN_HEADER
bool startDisplayStream(int x, int y);
void stopDisplayStream();
void* retrieveDisplayPixels(uint32_t* size); /*void* needs to be converted to float**/ 
ANDROID_END_HEADER