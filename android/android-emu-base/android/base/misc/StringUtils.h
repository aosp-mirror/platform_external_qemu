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

#include "android/base/StringView.h"

#include <functional>
#include <iterator>
#include <sstream>

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

// Joins all elements from the |range| into a single string, using |delim|
// as a delimiter
template <class Range, class Delimiter>
std::string join(const Range& range, const Delimiter& delim) {
    std::ostringstream out;

    // make sure we use the ADL versions of begin/end for custom ranges
    using std::begin;
    using std::end;
    auto it = begin(range);
    const auto itEnd = end(range);
    if (it != itEnd) {
        out << *it;
        for (++it; it != itEnd; ++it) {
            out << delim << *it;
        }
    }

    return out.str();
}

// Convenience version of join() with delimiter set to a single comma
template <class Range>
std::string join(const Range& range) {
    return join(range, ',');
}

// Returns a version of |in| with whitespace trimmed from the front/end
std::string trim(const std::string& in);

bool startsWith(StringView string, StringView prefix);
bool endsWith(StringView string, StringView suffix);

// Iterates over a string's parts using |splitBy| as a delimiter.
// |splitBy| must be a nonempty string well, or it's a no-op.
// Otherwise, |func| is called on each of the splits, excluding the
// characters that are part of |splitBy|.  If two |splitBy|'s occur in a row,
// |func| will be called on a StringView("") in between. See
// StringUtils_unittest.cpp for the full story.
template <class Func>
void split(StringView str, StringView splitBy, Func func) {
    if (splitBy.empty()) return;

    size_t splitSize = splitBy.size();
    size_t begin = 0;
    size_t end = str.find(splitBy);

    while (true) {
        func(str.substrAbs(begin, end));
        if (end == std::string::npos) return;
        begin = end + splitSize;
        end = str.find(splitBy, begin);
    }
}

}  // namespace base
}  // namespace android
