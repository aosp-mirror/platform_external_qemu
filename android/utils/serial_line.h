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

#pragma once

#include "android/utils/compiler.h"
#include "android/base/Limits.h"

// This is a C wrapper for a SerialLine abstraction, based
// on QEMU's character device (char.h)

ANDROID_BEGIN_HEADER

typedef struct CSerialLine {} CSerialLine;

typedef int SLCanReadHandler(void* opaque);
typedef void SLReadHandler(void* opaque, const uint8_t* buf, int size);

extern int android_serialline_write(CSerialLine* sl, const uint8_t* data, int len);
extern void android_serialline_addhandlers(CSerialLine* sl, void* opaque,
                                            SLCanReadHandler* canReadCallback,
                                            SLReadHandler* readCallback);

ANDROID_END_HEADER
