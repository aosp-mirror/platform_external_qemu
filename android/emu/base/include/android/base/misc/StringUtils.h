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

#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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
char* strDup(std::string_view str);

// Returns true iff |haystack| contains |needle|.
bool strContains(std::string_view haystack, const char* needle);

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

bool startsWith(std::string_view string, std::string_view prefix);
bool endsWith(std::string_view string, std::string_view suffix);

// Iterates over a string's parts using |splitBy| as a delimiter.
// |splitBy| must be a nonempty string well, or it's a no-op.
// Otherwise, |func| is called on each of the splits, excluding the
// characters that are part of |splitBy|.  If two |splitBy|'s occur in a row,
// |func| will be called on a std::string_view("") in between. See
// StringUtils_unittest.cpp for the full story.
template <class Func>
void split(std::string_view str, std::string_view splitBy, Func func) {
    if (splitBy.empty()) return;

    size_t splitSize = splitBy.size();
    size_t begin = 0;
    size_t end = str.find(splitBy);

    while (true) {
        func(str.substr(begin, end - begin));
        if (end == std::string::npos) return;
        begin = end + splitSize;
        end = str.find(splitBy, begin);
    }
}

// Tokenlize a string using |splitBy| as a delimiter.
// |splitBy| must be a nonempty string well, or it's a no-op.
// Whitespace is removed.
// Note: make sure out is an empty vector. It will be cleared
// before store the result tokens
void splitTokens(const std::string& input,
                 std::vector<std::string>* out,
                 std::string_view splitBy);

// Splits a string into a vector of strings.
//
// The string is split at each occurrence of a character in delimiters.
//
// The empty string is not a valid delimiter list.
std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiters);

// Trims whitespace off both ends of the given string.
std::string Trim(const std::string& s);

// Joins a container of things into a single string, using the given separator.
template <typename ContainerT, typename SeparatorT>
std::string Join(const ContainerT& things, SeparatorT separator) {
    if (things.empty()) {
        return "";
    }

    std::ostringstream result;
    result << *things.begin();
    for (auto it = std::next(things.begin()); it != things.end(); ++it) {
        result << separator << *it;
    }
    return result.str();
}

// We instantiate the common cases in strings.cpp.
extern template std::string Join(const std::vector<std::string>&, char);
extern template std::string Join(const std::vector<const char*>&, char);
extern template std::string Join(const std::vector<std::string>&,
                                 const std::string&);
extern template std::string Join(const std::vector<const char*>&,
                                 const std::string&);

// Tests whether 's' starts with 'prefix'.
bool StartsWith(std::string_view s, std::string_view prefix);
bool StartsWith(std::string_view s, char prefix);
bool StartsWithIgnoreCase(std::string_view s, std::string_view prefix);

// Tests whether 's' ends with 'suffix'.
bool EndsWith(std::string_view s, std::string_view suffix);
bool EndsWith(std::string_view s, char suffix);
bool EndsWithIgnoreCase(std::string_view s, std::string_view suffix);

// Tests whether 'lhs' equals 'rhs', ignoring case.
bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs);

// Removes `prefix` from the start of the given string and returns true (if
// it was present), false otherwise.
inline bool ConsumePrefix(std::string_view* s, std::string_view prefix) {
    if (!StartsWith(*s, prefix))
        return false;
    s->remove_prefix(prefix.size());
    return true;
}

// Removes `suffix` from the end of the given string and returns true (if
// it was present), false otherwise.
inline bool ConsumeSuffix(std::string_view* s, std::string_view suffix) {
    if (!EndsWith(*s, suffix))
        return false;
    s->remove_suffix(suffix.size());
    return true;
}

// Replaces `from` with `to` in `s`, once if `all == false`, or as many times as
// there are matches if `all == true`.
[[nodiscard]] std::string StringReplace(std::string_view s,
                                        std::string_view from,
                                        std::string_view to,
                                        bool all);
}  // namespace base
}  // namespace android
