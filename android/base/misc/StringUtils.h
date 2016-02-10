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

#pragma once

#include "android/base/containers/StringVector.h"
#include "android/base/String.h"
#include "android/base/StringView.h"

#include <stddef.h>

#ifdef _WIN32
// Returns a pointer to the first occurrence of |needle| in |haystack|, or a
// NULL pointer if |needle| is not part of |haystack|.
// Intentionally in global namespace. This is already provided by the system
// C library on Linux and OS X.
extern "C" const void* memmem(const void* haystack, size_t haystack_len,
                              const void* needle, size_t needlelen);
#endif  // _WIN32

namespace android {
namespace base {

// Copy the content of |view| into a new heap-allocated zero-terminated
// C string. Caller must free() the result.
char* strDup(StringView str);

// Returns true iff |haystack| contains |needle|.
bool strContains(StringView haystack, const char* needle);

// Sort an array of String instances.
void sortStringArray(String* strings, size_t n);

// Sort a StringVector instance normally. Required because Minw64 qsort()
// does not work with 3 items!!
inline void sortStringVector(StringVector* strings) {
    sortStringArray(&(*strings)[0], strings->size());
}

}  // namespace base
}  // namespace android
