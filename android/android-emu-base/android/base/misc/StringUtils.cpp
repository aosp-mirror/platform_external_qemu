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

#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <algorithm>
#include <string>
#include <vector>

#ifdef _WIN32
const void* memmem(const void* haystack, size_t haystackLen,
                   const void* needle, size_t needleLen) {
    if (!haystack || !needle) {
        return nullptr;
    }

    const auto it = std::search(
                        static_cast<const char*>(haystack),
                        static_cast<const char*>(haystack) + haystackLen,
                        static_cast<const char*>(needle),
                        static_cast<const char*>(needle) + needleLen);
    return it == static_cast<const char*>(haystack) + haystackLen
            ? nullptr
            : it;
}
#endif  // _WIN32

namespace android {
namespace base {

char* strDup(StringView view) {
    // Same as strdup(str.c_str()) but avoids a strlen() call.
    char* ret = static_cast<char*>(malloc(view.size() + 1u));
    ::memcpy(ret, view.data(), view.size());
    ret[view.size()] = '\0';
    return ret;
}

bool strContains(StringView haystack, const char* needle) {
    return ::memmem(haystack.data(), haystack.size(), needle,
                    ::strlen(needle)) != nullptr;
}

std::string Trim(const std::string& s) {
    std::string result;

    if (s.size() == 0) {
        return result;
    }

    size_t start_index = 0;
    size_t end_index = s.size() - 1;

    // Skip initial whitespace.
    while (start_index < s.size()) {
        if (!isspace(s[start_index])) {
            break;
        }
        start_index++;
    }

    // Skip terminating whitespace.
    while (end_index >= start_index) {
        if (!isspace(s[end_index])) {
            break;
        }
        end_index--;
    }

    // All spaces, no beef.
    if (end_index < start_index) {
        return "";
    }
    // Start_index is the first non-space, end_index is the last one.
    return s.substr(start_index, end_index - start_index + 1);
}

std::string trim(const std::string& in) {
    return Trim(in);
}

bool StartsWith(std::string_view s, std::string_view prefix) {
    return s.substr(0, prefix.size()) == prefix;
}

bool startsWith(StringView string, StringView prefix) {
    return string.size() >= prefix.size() &&
            memcmp(string.data(), prefix.data(), prefix.size()) == 0;
}

bool endsWith(StringView string, StringView suffix) {
    return string.size() >= suffix.size() &&
            memcmp(string.data() + string.size() - suffix.size(),
                   suffix.data(), suffix.size()) == 0;
}

void splitTokens(const std::string& input,
                 std::vector<std::string>* out,
                 StringView splitBy) {
    auto removeWhiteSpace = [out](StringView strView) {
        std::string s = strView.str();
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        out->push_back(s);
    };
    out->clear();
    split(input, splitBy, removeWhiteSpace);
}

#define CHECK_NE(a, b) \
    if ((a) == (b))    \
        abort();

std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiters) {
    CHECK_NE(delimiters.size(), 0U);

    std::vector<std::string> result;

    size_t base = 0;
    size_t found;
    while (true) {
        found = s.find_first_of(delimiters, base);
        result.push_back(s.substr(base, found - base));
        if (found == s.npos)
            break;
        base = found + 1;
    }

    return result;
}

// These cases are probably the norm, so we mark them extern in the header to
// aid compile time and binary size.
template std::string Join(const std::vector<std::string>&, char);
template std::string Join(const std::vector<const char*>&, char);
template std::string Join(const std::vector<std::string>&, const std::string&);
template std::string Join(const std::vector<const char*>&, const std::string&);

bool StartsWith(std::string_view s, char prefix) {
    return !s.empty() && s.front() == prefix;
}

bool StartsWithIgnoreCase(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() &&
           strncasecmp(s.data(), prefix.data(), prefix.size()) == 0;
}

bool EndsWith(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           s.substr(s.size() - suffix.size(), suffix.size()) == suffix;
}

bool EndsWith(std::string_view s, char suffix) {
    return !s.empty() && s.back() == suffix;
}

bool EndsWithIgnoreCase(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           strncasecmp(s.data() + (s.size() - suffix.size()), suffix.data(),
                       suffix.size()) == 0;
}

bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) {
    return lhs.size() == rhs.size() &&
           strncasecmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

std::string StringReplace(std::string_view s,
                          std::string_view from,
                          std::string_view to,
                          bool all) {
    if (from.empty())
        return std::string(s);

    std::string result;
    std::string_view::size_type start_pos = 0;
    do {
        std::string_view::size_type pos = s.find(from, start_pos);
        if (pos == std::string_view::npos)
            break;

        result.append(s.data() + start_pos, pos - start_pos);
        result.append(to.data(), to.size());

        start_pos = pos + from.size();
    } while (all);
    result.append(s.data() + start_pos, s.size() - start_pos);
    return result;
}

}  // namespace base
}  // namespace android
