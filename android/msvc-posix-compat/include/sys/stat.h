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

// This sets up a series of compatibility defines that enable compilation
// of qemu on windows with clang-cl.
#include_next <sys/stat.h>

#define fstat64 _fstat64
#define stat _stati64
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE

