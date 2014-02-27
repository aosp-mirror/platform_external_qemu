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

#ifndef ANDROID_BASE_FILES_PATH_UTIL_H
#define ANDROID_BASE_FILES_PATH_UTIL_H

#include "android/base/containers/StringVector.h"
#include "android/base/String.h"

namespace android {
namespace base {

// Utility functions to manage file paths. None of these should touch the
// file system. All methods must be static.
class PathUtils {
public:
    // An enum listing the supported host file system types.
    // HOST_POSIX means a Posix-like file system.
    // HOST_WIN32 means a Windows-like file system.
    // HOST_TYPE means the current host type (one of the above).
    // NOTE: If you update this list, modify kHostTypeCount below too.
    enum HostType {
        HOST_POSIX = 0,
        HOST_WIN32 = 1,
#ifdef _WIN32
        HOST_TYPE = HOST_WIN32,
#else
        HOST_TYPE = HOST_POSIX,
#endif
    };

    // The number of distinct items in the HostType enumeration above.
    static const int kHostTypeCount = 2;

    // Return true if |ch| is a directory separator for a given |hostType|.
    static bool isDirSeparator(int ch, HostType hostType);

    // Return true if |ch| is a directory separator for the current platform.
    static inline bool isDirSeparator(int ch) {
        return isDirSeparator(ch, HOST_TYPE);
    }

    // Return true if |ch| is a path separator for a given |hostType|.
    static bool isPathSeparator(int ch, HostType hostType);

    // Return true if |ch| is a path separator for the current platform.
    static inline bool isPathSeparator(int ch) {
        return isPathSeparator(ch, HOST_TYPE);
    }

    // If |path} starts with a root prefix, return its size in bytes, or
    // 0 otherwise. The definition of valid root prefixes depends on the
    // value of |hostType|. For HOST_POSIX, it's any path that begins
    // with a slash (/). For HOST_WIN32, the following prefixes are
    // recognized:
    //    <drive>:
    //    <drive>:<sep>
    //    <sep><sep>volumeName<sep>
    static size_t rootPrefixSize(const char* path, HostType hostType);

    // Return the root prefix for the current platform. See above for
    // documentation.
    static inline size_t rootPrefixSize(const char* path) {
        return rootPrefixSize(path, HOST_TYPE);
    }

    // Return true iff |path| is an absolute path for a given |hostType|.
    static bool isAbsolute(const char* path, HostType hostType);

    // Return true iff |path| is an absolute path for the current host.
    static inline bool isAbsolute(const char* path) {
        return isAbsolute(path, HOST_TYPE);
    }

    // Decompose |path| into individual components. If |path| has a root
    // prefix, it will always be the first component. I.e. for Posix
    // systems this will be '/' (for absolute paths). For Win32 systems,
    // it could be 'C:" (for a path relative to a root volume) or "C:\"
    // for an absolute path from volume C).,
    // On success, return true and sets |out| to a vector of strings,
    // each one being a path component (prefix or subdirectory or file
    // name). Directory separators do not appear in components, except
    // for the root prefix, if any.
    static StringVector decompose(const char* path, HostType hostType);

    // Decompose |path| into individual components for the host platform.
    // See comments above for more details.
    static inline StringVector decompose(const char* path) {
        return decompose(path, HOST_TYPE);
    }

    // Recompose a path from individual components into a file path string.
    // |components| is a vector of strings, and |hostType| the target
    // host type to use. Return a new file path string. Note that if the
    // first component is a root prefix, it will be kept as is, i.e.:
    //   [ 'C:', 'foo' ] -> 'C:foo' on Win32, but not Posix where it will
    // be 'C:/foo'.
    static String recompose(const StringVector& components,
                                 HostType hostType);

    // Recompose a path from individual components into a file path string
    // for the current host. |components| is a vector os strings.
    // Returns a new file path string.
    static inline String recompose(const StringVector& components) {
        return recompose(components, HOST_TYPE);
    }

    // Given a list of components returned by decompose(), simplify it
    // by removing instances of '.' and '..' when that makes sense.
    // Note that it is not possible to simplify initial instances of
    // '..', i.e. "foo/../../bar" -> "../bar"
    static void simplifyComponents(StringVector* components);
};

// Useful shortcuts to avoid too much typing.
static const PathUtils::HostType kHostPosix = PathUtils::HOST_POSIX;
static const PathUtils::HostType kHostWin32 = PathUtils::HOST_WIN32;
static const PathUtils::HostType kHostType = PathUtils::HOST_TYPE;

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_FILES_PATH_UTIL_H
