// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/PathUtils.h"

#include <string.h>

namespace android {
namespace base {

// static
bool PathUtils::isDirSeparator(int ch, HostType hostType) {
    return (ch == '/') || (hostType == HOST_WIN32 && ch == '\\');
}


// static
bool PathUtils::isPathSeparator(int ch, HostType hostType) {
    return (hostType == HOST_POSIX && ch == ':') ||
            (hostType == HOST_WIN32 && ch == ';');
}

// static
size_t PathUtils::rootPrefixSize(const char* path, HostType hostType) {
    if (!path || !path[0])
        return 0;

    if (hostType != HOST_WIN32)
        return (path[0] == '/') ? 1U : 0U;

    size_t result = 0;
    if (path[1] == ':') {
        int ch = path[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
            result = 2U;
    } else if (!strncmp(path, "\\\\.\\", 4) ||
            !strncmp(path, "\\\\?\\", 4)) {
        // UNC prefixes.
        return 4U;
    } else if (isDirSeparator(path[0], hostType)) {
        result = 1;
        if (isDirSeparator(path[1], hostType)) {
            result = 2;
            while (path[result] && !isDirSeparator(path[result], HOST_WIN32))
                result++;
        }
    }
    if (result && path[result] && isDirSeparator(path[result], HOST_WIN32))
        result++;

    return result;
}

// static
bool PathUtils::isAbsolute(const char* path, HostType hostType) {
    size_t prefixSize = rootPrefixSize(path, hostType);
    if (!prefixSize) {
        return false;
    }
    if (hostType != HOST_WIN32) {
        return true;
    }
    return isDirSeparator(path[prefixSize - 1], HOST_WIN32);
}

// static
StringVector PathUtils::decompose(const char* path, HostType hostType) {
    StringVector result;
    if (!path || !path[0])
        return result;

    size_t prefixLen = rootPrefixSize(path, hostType);
    if (prefixLen) {
        result.push_back(String(path, prefixLen));
        path += prefixLen;
    }
    for (;;) {
        const char* p = path;
        while (*p && !isDirSeparator(*p, hostType))
            p++;
        if (p > path) {
            result.push_back(String(path, p - path));
        }
        if (!*p) {
            break;
        }
        path = p + 1;
    }
    return result;
}

// static
String PathUtils::recompose(const StringVector& components,
                            HostType hostType) {
    const char dirSeparator = (hostType == HOST_WIN32) ? '\\' : '/';
    String result;
    size_t capacity = 0;
    // To reduce memory allocations, compute capacity before doing the
    // real append.
    for (size_t n = 0; n < components.size(); ++n) {
        if (n)
            capacity++;
        capacity += components[n].size();
    }

    result.reserve(capacity);

    bool addSeparator = false;
    for (size_t n = 0; n < components.size(); ++n) {
        const String& component = components[n];
        if (addSeparator)
            result += dirSeparator;
        addSeparator = true;
        if (n == 0) {
            size_t prefixLen = rootPrefixSize(component.c_str(), hostType);
            if (prefixLen > 0) {
                addSeparator = false;
            }
        }
        result += components[n];
    }
    return result;
}

// static
void PathUtils::simplifyComponents(StringVector* components) {
    StringVector stack;
    for (StringVector::const_iterator it = components->begin();
            it != components->end();
            ++it) {
        if (*it == ".") {
            // Ignore any instance of '.' from the list.
            continue;
        }
        if (*it == "..") {
            // Handling of '..' is specific: if there is a item on the
            // stack that is not '..', then remove it, otherwise push
            // the '..'.
            if (!stack.empty() && stack[stack.size() -1U] != "..") {
                stack.resize(stack.size() - 1U);
            } else {
                stack.push_back(*it);
            }
            continue;
        }
        // If not a '..', just push on the stack.
        stack.push_back(*it);
    }
    if (stack.empty())
        stack.push_back(".");

    components->swap(&stack);
}

}  // namespace base
}  // namespace android
