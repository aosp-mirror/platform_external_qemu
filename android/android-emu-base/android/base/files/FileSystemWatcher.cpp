// Copyright 2022` The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/files/FileSystemWatcher.h"

#include <algorithm>  // for set_difference
#include <string>     // for basic_string
#include <vector>     // for vector

#include "android/base/system/System.h"  // for System

namespace android {
namespace base {

using Path = std::filesystem::path;


}  // namespace base
}  // namespace android