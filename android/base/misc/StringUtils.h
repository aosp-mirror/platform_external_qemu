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

#ifndef ANDROID_BASE_MISC_STRING_UTILS_H
#define ANDROID_BASE_MISC_STRING_UTILS_H

#include "android/base/containers/StringVector.h"
#include "android/base/String.h"

namespace android {
namespace base {

// Sort an array of String instances.
void sortStringArray(String* strings, size_t n);

// Sort a StringVector instance normally. Required because Minw64 qsort()
// does not work with 3 items!!
inline void sortStringVector(StringVector* strings) {
    sortStringArray(&(*strings)[0], strings->size());
}

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_MISC_STRING_UTILS_H
