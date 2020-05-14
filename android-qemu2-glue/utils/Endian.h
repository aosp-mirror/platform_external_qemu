// Copyright 2020 The Android Open Source Project
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

#if defined(__APPLE__) || defined(_WIN32)

#define __BIG_ENDIAN 0x1000
#define __LITTLE_ENDIAN 0x0001
#define __BYTE_ORDER __LITTLE_ENDIAN

#else

#include <endian.h>

#endif