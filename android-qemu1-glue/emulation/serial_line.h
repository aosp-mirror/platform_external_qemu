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
#include "qemu-common.h"

ANDROID_BEGIN_HEADER

// Create a new CSerialLine instance that wraps a CharDriverState |cs|.
CSerialLine* android_serialline_from_cs(CharDriverState* cs);

// Extract the QEMU1 CharDriverState instance wrapped by a CSerialLine
// instance |sl|.
CharDriverState* android_serialline_get_cs(CSerialLine* sl);

// Extract the CharDriverState instance wrapped by |sl|, but also
// destroys the CSerialLine instance.
CharDriverState* android_serialline_release_cs(CSerialLine* sl);

ANDROID_END_HEADER
