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

#pragma once

#include "aemu/base/async/Looper.h"

// An implementation of android::base::Looper based on the QEMU event loop.
namespace android {
namespace qemu {

// Create a new android::base::Looper instance that is implemented through
// the QEMU main event loop. There is only one instance, so any call will
// return an object corresponding to the same global state, even if they
// are different instances!
android::base::Looper* createLooper();

// Skip timer ops on exit to prevent crashes in timer related code.
void skipTimerOps();

}  // namespace qemu
}  // namespace android
