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
//
// SerialLine allows one to write data into the connection and supply a
// callback for reading.
// To send some data, just call |android_serialline_write| on a line
// When there's some data to read, the callbacks supplied in
// android_serialline_addhandlers() will be called in this order:
//    SLCanReadHandler(opaque)
//    SLReadHandler(opaque, data, len)
// |opaque| is the value passed in second argument of
// android_serialline_addhandlers() - some user state for the callbacks

ANDROID_BEGIN_HEADER

// A handle to the serial line object
typedef struct CSerialLine {} CSerialLine;

// Read callbacks for the serial line

// return a number of bytes read handler can consume now.
typedef int SLCanReadHandler(void* opaque);
// read the data - |size| bytes from memory at |buf|
typedef void SLReadHandler(void* opaque, const uint8_t* buf, int size);

// sets read callbacks on a specific serial line object.
// |opaque| is some user state to be passed to the callbacks
extern void android_serialline_addhandlers(CSerialLine* sl, void* opaque,
                                           SLCanReadHandler* canReadCallback,
                                           SLReadHandler* readCallback);

// try to send |len| bytes of |data| through the serial line,
// returns the number of bytes that went through, or negative number on error
extern int android_serialline_write(CSerialLine* sl, const uint8_t* data, int len);

ANDROID_END_HEADER
