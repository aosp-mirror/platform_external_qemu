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

#ifndef QEMU_ANDROID_UTILS_CURL_H
#define QEMU_ANDROID_UTILS_CURL_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// CURL library initialization/cleanup helpers
// they make sure it is initialized properly, and only once
// NOTE: not thread-safe, run only on a main thread!

extern void curl_init();
extern void curl_cleanup();

ANDROID_END_HEADER

#endif //QEMU_ANDROID_UTILS_CURL_H