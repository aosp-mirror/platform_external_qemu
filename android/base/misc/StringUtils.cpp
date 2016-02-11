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
#include <string.h>

#ifdef _WIN32
const void* memmem(const void* haystack, size_t haystackLen,
                   const void* needle, size_t needleLen) {
    if (needleLen == 0 ) {
        return haystack;
    }

    if (haystackLen < needleLen) {
        return NULL;
    }

    const char* haystackPos = (const char*)haystack;
    const char* haystackEnd = haystackPos + (haystackLen - needleLen) + 1;
    for (; haystackPos < haystackEnd; haystackPos++) {
        if (0==memcmp(haystackPos, needle, needleLen)) {
            return haystackPos;
        }
    }
    return NULL;
}
#endif  // _WIN32

namespace android {
namespace base {

char* strDup(StringView view) {
    // Same as strdup(str.c_str()) but avoids a strlen() call.
    char* ret = static_cast<char*>(malloc(view.size() + 1u));
    ::memcpy(ret, view.c_str(), view.size());
    ret[view.size()] = '\0';
    return ret;
}

bool strContains(StringView haystack, const char* needle) {
    return ::memmem(haystack.c_str(), haystack.size(),
                    needle, ::strlen(needle)) != nullptr;
}

}  // namespace base
}  // namespace android
