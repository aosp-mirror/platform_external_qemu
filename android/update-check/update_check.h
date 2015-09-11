// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef QEMU_ANDROID_UPDATECHECK_H
#define QEMU_ANDROID_UPDATECHECK_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

extern void android_checkForUpdates(const char* homePath);

ANDROID_END_HEADER

#endif  // QEMU_ANDROID_UPDATECHECK_H
