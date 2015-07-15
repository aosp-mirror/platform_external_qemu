// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _ANDROID_EXT4_RESIZE_H
#define _ANDROID_EXT4_RESIZE_H

#include <android/utils/compiler.h>
#include <android/base/Limits.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// resize the partition in |partitionPath| to be newSize;
// |partitionPath| should hold the full, null-terminated path to the
// desired partition
// |newSize| should be the desired new size of the partition in bytes
void resizeExt4Partition(char * partitionPath, int64_t newSize);

ANDROID_END_HEADER

#endif /* _ANDROID_EXT4_RESIZE_H */
