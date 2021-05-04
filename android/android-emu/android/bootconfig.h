// Copyright 2021 The Android Open Source Project
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

#include <string>
#include <vector>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// [src ramdisk][bootconfig][padding][size(le32)][csum(le32)][#BOOTCONFIG\n]
//                                   ^ 4 byte aligned

std::vector<char> buildBootconfigBlob(const size_t srcSize,
                                      const std::vector<std::pair<std::string, std::string>>& bootconfig);

int createRamdiskWithBootconfig(const char* srcRamdiskPath,
                                const char* dstRamdiskPath,
                                const std::vector<std::pair<std::string, std::string>>& bootconfig);

ANDROID_END_HEADER
