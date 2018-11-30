// Copyright 2018 The Android Open Source Project
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

#include <vector>
#include "android/utils/compiler.h"
ANDROID_BEGIN_HEADER
void opengl_object_count_inc(int type);
void opengl_object_count_dec(int type);
ANDROID_END_HEADER

namespace android {
namespace opengl {
void getOpenGLObjectCounts(std::vector<int>* vec);
}  // namespace opengl
}  // namespace android
