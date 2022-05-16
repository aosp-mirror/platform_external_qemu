/* Copyright (C) 2016 The Android Open Source Project
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

// Name of environment variable used to control the name of the device
// to use, when it is not /dev/kvm
#define KVM_DEVICE_NAME_ENV  "ANDROID_EMULATOR_KVM_DEVICE"

ANDROID_END_HEADER
