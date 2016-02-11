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

#include "android/base/system/System.h"

#include <iterator>

#include <assert.h>
#include <string.h>

namespace android {
namespace base {

const char* const PathUtils::kExeNameSuffixes[kHostTypeCount] = { "", ".exe" };

const char* const PathUtils::kExeNameSuffix =
        PathUtils::kExeNameSuffixes[PathUtils::HOST_TYPE];

std::string PathUtils::toExecutableName(StringView baseName,
                                        HostType hostType) {
    return static_cast<std::string>(baseName).append(
            kExeNameSuffixes[hostType]);
}

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
size_t PathUtils::rootPrefixSize(StringView path, HostType hostType) {
    if (path.empty())
        return 0;

    if (hostType != HOST_WIN32)
        return (path[0] == '/') ? 1U : 0U;

    size_t result = 0;
    if (path[1] == ':') {
        int ch = path[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
            result = 2U;
    } else if (!strncmp(path.begin(), "\\\\.\\", 4) ||
            !strncmp(path.begin(), "\\\\?\\", 4)) {
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
bool PathUtils::isAbsolute(StringView path, HostType hostType) {
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
StringView PathUtils::extension(const StringView path, HostType hostType) {
    using riter = std::reverse_iterator<StringView::const_iterator>;

    for (auto it = riter(path.end()), itEnd = riter(path.begin());
         it != itEnd; ++it) {
        if (*it == '.') {
            // reverse iterator stores a base+1, so decrement it when returning
            return StringView(std::prev(it.base()), path.end());
        }
        if (isDirSeparator(*it, hostType)) {
            // no extension here - we've found the end of file name already
            break;
        }
    }

    // either there's no dot in the whole path, or we found directory separator
    // first - anyway, there's no extension in this name
    return StringView();
}

// static
std::string PathUtils::removeTrailingDirSeparator(StringView path,
                                                  HostType hostType) {
    std::string result = path;
    size_t pathLen = result.size();
    // NOTE: Don't remove initial path separator for absolute paths.
    while (pathLen > 1U && isDirSeparator(result[pathLen - 1U], hostType)) {
        pathLen--;
    }
    result.resize(pathLen);
    return result;
}

// static
std::string PathUtils::addTrailingDirSeparator(StringView path,
                                               HostType hostType) {
    std::string result = path;
    if (result.size() > 0 && !isDirSeparator(result[result.size() - 1U])) {
        result += getDirSeparator(hostType);
    }
    return result;
}

// static
bool PathUtils::split(StringView path,
                      HostType hostType,
                      std::string* dirName,
                      std::string* baseName) {
    if (path.empty()) {
        return false;
    }

    // If there is a trailing directory separator, return an error.
    size_t end = path.size();
    if (isDirSeparator(path[end - 1U], hostType)) {
        return false;
    }

    // Find last separator.
    size_t prefixLen = rootPrefixSize(path, hostType);
    size_t pos = end;
    while (pos > prefixLen && !isDirSeparator(path[pos - 1U], hostType)) {
        pos--;
    }

    // Handle common case.
    if (pos > prefixLen) {
        if (dirName) {
            dirName->assign(path.begin(), pos);
        }
        if (baseName) {
            baseName->assign(path.begin() + pos, end - pos);
        }
        return true;
    }

    // If there is no directory separator, the path is a single file name.
    if (dirName) {
        if (!prefixLen) {
            dirName->assign(".");
        } else {
            dirName->assign(path.begin(), prefixLen);
        }
    }
    if (baseName) {
        baseName->assign(path.begin() + prefixLen, end - prefixLen);
    }
    return true;
}

// static
std::string PathUtils::join(StringView path1,
                            StringView path2,
                            HostType hostType) {
    if (path1.empty()) {
        return path2;
    }
    if (path2.empty()) {
        return path1;
    }
    if (isAbsolute(path2, hostType)) {
        return path2;
    }
    size_t prefixLen = rootPrefixSize(path1, hostType);
    std::string result(path1);
    size_t end = result.size();
    if (end > prefixLen && !isDirSeparator(result[end - 1U], hostType)) {
        result += getDirSeparator(hostType);
    }
    result += path2;
    return result;
}

// static
std::vector<std::string> PathUtils::decompose(StringView path,
                                              HostType hostType) {
    std::vector<std::string> result;
    if (path.empty())
        return result;

    size_t prefixLen = rootPrefixSize(path, hostType);
    auto it = path.begin();
    if (prefixLen) {
        result.push_back(std::string(it, prefixLen));
        it += prefixLen;
    }
    for (;;) {
        auto p = it;
        while (*p && !isDirSeparator(*p, hostType))
            p++;
        if (p > it) {
            result.push_back(std::string(it, p - it));
        }
        if (!*p) {
            break;
        }
        it = p + 1;
    }
    return result;
}

// static
std::string PathUtils::recompose(const std::vector<std::string>& components,
                                 HostType hostType) {
    const char dirSeparator = getDirSeparator(hostType);
    std::string result;
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
        const std::string& component = components[n];
        if (addSeparator)
            result += dirSeparator;
        addSeparator = true;
        if (n == 0) {
            size_t prefixLen = rootPrefixSize(component, hostType);
            if (prefixLen == component.size()) {
                addSeparator = false;
            }
        }
        result += components[n];
    }
    return result;
}

// static
void PathUtils::simplifyComponents(std::vector<std::string>* components) {
    std::vector<std::string> stack;
    for (std::vector<std::string>::const_iterator it = components->begin();
         it != components->end(); ++it) {
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

    components->swap(stack);
}

}  // namespace base
}  // namespace android
