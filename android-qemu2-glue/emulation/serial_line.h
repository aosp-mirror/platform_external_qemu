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

#include "android/emulation/serial_line.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// NOTE: Do not include "qemu-common.h" here because this fails
//       to compile when included from a C++ file due to other
//       issues in include/qemu/int28.h. Instead, use the
typedef struct Chardev Chardev;

// Call this during setup to inject QEMU2-specific SerialLine
// implementation into the process.
void qemu2_android_serialline_init(void);

// Create a new CSerialLine instance that wraps a Chardev |dev|.
CSerialLine* android_serialline_from_cs(Chardev* dev);

// Extract the QEMU2 Chardev instance wrapped by a CSerialLine
// instance |sl|.
Chardev* android_serialline_get_cs(CSerialLine* sl);

ANDROID_END_HEADER
