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
#include <stdint.h>
#include <stdlib.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidClipboardAgent {
    // Turns clipboard sharing on/off.
    void (*setEnabled)(bool enabled);

    // Registers a function to call when the guest's clipboard content changes
    // as a result of an action inside the guest. Note that multiple functions
    // can be registered.
    // |cb| is the pointer to the function
    void (*registerGuestClipboardCallback)(
        void(*cb)(void*, const uint8_t*, size_t), void* context);

    // Sets the contents of the guest clipboard.
    // |buf| is the content to pass, len is the number of bytes in |buf|.
    void (*setGuestClipboardContents)(
        const uint8_t* buf,
        size_t len);
} QAndroidClipboardAgent;

ANDROID_END_HEADER
