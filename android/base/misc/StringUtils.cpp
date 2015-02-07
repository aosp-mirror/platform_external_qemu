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

#include "android/base/misc/StringUtils.h"

#include "android/base/memory/QSort.h"
#include <stdlib.h>

namespace android {
namespace base {

struct StringQSortTraits {
    static int compare(const String* a, const String* b) {
        return ::strcmp(a->c_str(), b->c_str());
    }
    static void swap(String* a, String* b) {
        a->swap(b);
    }
};

void sortStringArray(String* strings, size_t count) {
    QSort<String, StringQSortTraits>::sort(strings, count);
}

}  // namespace base
}  // namespace android
