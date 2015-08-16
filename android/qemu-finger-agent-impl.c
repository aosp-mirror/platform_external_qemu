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

#include "android/finger-agent-impl.h"

#include "android/finger-agent.h"
#include "android/hw-fingerprint.h"

void finger_setTouch(bool isTouching, int touchId)
{
    if (isTouching) {
        android_hw_fingerprint_touch(touchId);
    } else {
        android_hw_fingerprint_remove();
    }
}
