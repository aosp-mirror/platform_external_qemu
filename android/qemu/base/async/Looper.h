// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_QEMU_BASE_ASYNC_LOOPER_H
#define ANDROID_QEMU_BASE_ASYNC_LOOPER_H

#include "android/base/async/Looper.h"

// An implementation of android::base::Looper based on the QEMU event loop.
namespace android {
namespace qemu {

// Create a new android::base::Looper instance that is implemented through
// the QEMU main event loop. There is only one instance, so any call will
// return an object corresponding to the same global state, even if they
// are different instances!
android::base::Looper* createLooper();

}  // namespace qemu
}  // namespace android

#endif  // ANDROID_QEMU_BASE_ASYNC_LOOPER_H
