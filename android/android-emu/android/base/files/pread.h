// Copyright 2017 The Android Open Source Project
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

#ifndef _WIN32
#include <unistd.h>
#else
#include <stdint.h>
#endif

namespace android {
namespace base {

#ifndef _WIN32
using ::pread;
#else
int64_t pread(int fd, void* buf, size_t count, int64_t offset);
#endif

}  // namespace base
}  // namespace android
