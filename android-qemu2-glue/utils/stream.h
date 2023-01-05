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
#include "aemu/base/utils/stream.h"

ANDROID_BEGIN_HEADER

// this pure C header has to be under 'extern "C"' for a C++ code
#include "qemu/typedefs.h"

// Create a new Stream instance that wraps a QEMUFile instance.
// This allows one to use it with other stream_xxx() functions
// defined in "android/utils/stream.h"
// |file| is a QEMUFile instance. Returns a new Stream instance.
Stream* stream_from_qemufile(QEMUFile* file);

ANDROID_END_HEADER
