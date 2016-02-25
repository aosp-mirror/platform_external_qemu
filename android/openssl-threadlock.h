// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Setup callbacks for thread locking required by OpenSSL.
// You must all android_openssl_threadlock_init before creating threads, and
// android_openssl_threadlock_cleanup at the end of the process.
bool android_openssl_threadlock_init();
void android_openssl_threadlock_cleanup();

ANDROID_END_HEADER
