// Copyright 2019 The Android Open Source Project
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

#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidHwControlAgent {
    void (*setBrightness)(const char* light_name, uint32_t brightness);
    uint32_t (*getBrightness)(const char* light_name);
} QAndroidHwControlAgent;

ANDROID_END_HEADER
