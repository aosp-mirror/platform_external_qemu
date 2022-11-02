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

#include "android-qemu2-glue/looper-qemu.h"

#include "android-qemu2-glue/base/async/Looper.h"
#include "aemu/base/async/Looper.h"
#include "android/utils/looper.h"

typedef ::Looper CLooper;

void qemu_looper_setForThread() {
    looper_setForThreadToOwn(
            reinterpret_cast<CLooper*>(::android::qemu::createLooper()));
}
