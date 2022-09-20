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

#pragma once

#include <stddef.h>                   // for size_t
#include <optional>
#include <string>                     // for string, basic_string
#include <string_view>
#include <utility>                    // for move, forward
#include <vector>                     // for vector

#include "android/base/Optional.h"    // for Optional

#ifdef __APPLE__

#define LIBSUFFIX ".dylib"

#else

#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#define LIBSUFFIX ".dll"

#else

#define LIBSUFFIX ".so"

#endif // !_WIN32 (linux)

#endif // !__APPLE__

namespace android {
namespace base {

// Helper to get a null-terminated const char* from a string_view.
// Only allocates if the string_view is not null terminated.
//
// Usage:
//
//      std::string_view myString = ...;
//      printf("Contents: %s\n", CStrWrapper(myString).c_str());
//
// c_str(...) constructs a temporary object that may allocate memory if the
// StringView is not null termianted.  The lifetime of the temporary object will
// be until the next sequence point (typically the next semicolon).  If the
// value needs to exist for longer than that, cache the instance.
//
//      std::string_view myString = ...;
//      auto myNullTerminatedString = CStrWrapper(myString).c_str();
//      functionAcceptingConstCharPointer(myNullTerminatedString);
//
class CStrWrapper {
public:
    CStrWrapper(std::string_view stringView) : mStringView(stringView) {}

    // Returns a null-terminated char*, potentially creating a copy to add a
    // null terminator.
    const char* get() {
        if (mStringView.back() == '\0') {
            return mStringView.data();
        } else {
            // Create the std::string copy on-demand.
            if (!mStringCopy) {
                mStringCopy.emplace(mStringView);
            }

            return mStringCopy->c_str();
        }
    }

    // Alias for get
    const char* c_str() {
        return get();
    }

    // Enable casting to const char*
    operator const char*() { return get(); }

private:
    const std::string_view mStringView;
    std::optional<std::string> mStringCopy;
};

inline CStrWrapper c_str(std::string_view stringView) {
    return CStrWrapper(stringView);
}

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

    // Suffixes for an executable file (.exe on Windows, empty otherwise)
    static const char* const kExeNameSuffixes[kHostTypeCount];

    // Suffixe for an executable file on the current platform
    static const char* const kExeNameSuffix;

    // Returns the executable name for a base name |baseName|
    static std::string toExecutableName(std::string_view baseName,
                                        HostType hostType);

    static std::string toExecutableName(std::string_view baseName) {
        return toExecutableName(baseName, HOST_TYPE);
    }

    // Return true if |ch| is a directory separator for a given |hostType|.
    static bool isDirSeparator(int ch, HostType hostType);

    // Return true if |ch| is a directory separator for the current platform.
    static bool isDirSeparator(int ch) {
        return isDirSeparator(ch, HOST_TYPE);
    }

    // Return true if |ch| is a path separator for a given |hostType|.
    static bool isPathSeparator(int ch, HostType hostType);

    // Return true if |ch| is a path separator for the current platform.
    static bool isPathSeparator(int ch) {
        return isPathSeparator(ch, HOST_TYPE);
    }

    // Return the directory separator character for a given |hostType|
    static char getDirSeparator(HostType hostType) {
        return (hostType == HOST_WIN32) ? '\\' : '/';
    }

    // Remove trailing separators from a |path| string, for a given |hostType|.
    static std::string_view removeTrailingDirSeparator(std::string_view path,
                                                       HostType hostType);

    // Remove trailing separators from a |path| string for the current host.
    static std::string_view removeTrailingDirSeparator(std::string_view path) {
        return removeTrailingDirSeparator(path, HOST_TYPE);
    }

    // Add a trailing separator if needed.
    static std::string addTrailingDirSeparator(std::string_view path,
                                               HostType hostType);
    static std::string addTrailingDirSeparator(const std::string& path, HostType hostType);
    static std::string addTrailingDirSeparator(const char* path, HostType hostType);


    // Add a trailing separator if needed.
    static std::string addTrailingDirSeparator(std::string_view path) {
        return addTrailingDirSeparator(path, HOST_TYPE);
    }
    static std::string addTrailingDirSeparator(const std::string& path);

    // If |path| starts with a root prefix, return its size in bytes, or
    // 0 otherwise. The definition of valid root prefixes depends on the
    // value of |hostType|. For HOST_POSIX, it's any path that begins
    // with a slash (/). For HOST_WIN32, the following prefixes are
    // recognized:
    //    <drive>:
    //    <drive>:<sep>
    //    <sep><sep>volumeName<sep>
    static size_t rootPrefixSize(const std::string& path, HostType hostType);

    // Return the root prefix for the current platform. See above for
    // documentation.
    static size_t rootPrefixSize(const std::string& path) {
        return rootPrefixSize(path, HOST_TYPE);
    }

    static size_t rootPrefixSize(const char* path) {
        return path ? rootPrefixSize(std::string(path), HOST_TYPE) : 0;
    }

    static size_t rootPrefixSize(const char* path, HostType hostType) {
        return path ? rootPrefixSize(std::string(path), hostType) : 0;
    }

    // Return true iff |path| is an absolute path for a given |hostType|.
    static bool isAbsolute(const std::string& path, HostType hostType);

    // Return true iff |path| is an absolute path for the current host.
    static bool isAbsolute(const std::string& path) {
        return isAbsolute(path, HOST_TYPE);
    }

    static bool isAbsolute(const char* path) {
        return isAbsolute(path, HOST_TYPE);
    }

    static bool isAbsolute(const char* path, HostType hostType) {
        return path ? isAbsolute(std::string(path), hostType) : false;
    }

    // Return an extension part of the name/path (the part of the name after
    // last dot, including the dot. E.g.:
    //  "file.name" -> ".name"
    //  "file" -> ""
    //  "file." -> "."
    //  "/full/path.png" -> ".png"
    static std::string_view extension(const std::string& path,
                                      HostType hostType = HOST_TYPE);

    // Split |path| into a directory name and a file name. |dirName| and
    // |baseName| are optional pointers to strings that will receive the
    // corresponding components on success. |hostType| is a host type.
    // Return true on success, or false on failure.
    // Note that unlike the Unix 'basename' command, the command will fail
    // if |path| ends with directory separator or is a single root prefix.
    // Windows root prefixes are fully supported, which means the following:
    //
    //     /            -> error.
    //     /foo         -> '/' + 'foo'
    //     foo          -> '.' + 'foo'
    //     <drive>:     -> error.
    //     <drive>:foo  -> '<drive>:' + 'foo'
    //     <drive>:\foo -> '<drive>:\' + 'foo'
    //
    static bool split(std::string_view path,
                      HostType hostType,
                      std::string_view* dirName,
                      std::string_view* baseName);

    // A variant of split() for the current process' host type.
    static bool split(std::string_view path,
                      std::string_view* dirName,
                      std::string_view* baseName) {
        return split(path, HOST_TYPE, dirName, baseName);
    }

    // Join two path components together. Note that if |path2| is an
    // absolute path, this function returns a copy of |path2|, otherwise
    // the result will be the concatenation of |path1| and |path2|, if
    // |path1| doesn't end with a directory separator, a |hostType| specific
    // one will be inserted between the two paths in the result.
    static std::string join(const std::string& path1,
                            const std::string& path2,
                            HostType hostType);

    // A variant of join() for the current process' host type.
    static std::string join(const std::string& path1, const std::string& path2) {
        return join(path1, path2, HOST_TYPE);
    }

    // A convenience function to join a bunch of paths at once
    template <class... Paths>
    static std::string join(const std::string& path1,
                            const std::string& path2,
                            Paths&&... paths) {
        return join(path1, join(path2, std::forward<Paths>(paths)...));
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
    static std::vector<std::string_view> decompose(std::string_view path,
                                                   HostType hostType);
    static std::vector<std::string> decompose(std::string&& path,
                                              HostType hostType);
    static std::vector<std::string_view> decompose(const char* path,
                                                   HostType hostType) {
        return decompose(std::string_view(path), hostType);
    }
    static std::vector<std::string> decompose(const std::string& path,
                                              HostType hostType);

    template <class String>
    static std::vector<String> decompose(const String& path,
                                         HostType hostType);

    // Decompose |path| into individual components for the host platform.
    // See comments above for more details.
    static std::vector<std::string_view> decompose(std::string_view path) {
        return decompose(path, HOST_TYPE);
    }
    static std::vector<std::string> decompose(std::string&& path) {
        return decompose(std::move(path), HOST_TYPE);
    }
    static std::vector<std::string_view> decompose(const char* path) {
        return decompose(std::string_view(path));
    }

    // Recompose a path from individual components into a file path string.
    // |components| is a vector of strings, and |hostType| the target
    // host type to use. Return a new file path string. Note that if the
    // first component is a root prefix, it will be kept as is, i.e.:
    //   [ 'C:', 'foo' ] -> 'C:foo' on Win32, but not Posix where it will
    // be 'C:/foo'.
    static std::string recompose(
            const std::vector<std::string_view>& components,
            HostType hostType);
    static std::string recompose(const std::vector<std::string>& components,
                                 HostType hostType);
    template <class String>
    static std::string recompose(const std::vector<String>& components,
                                 HostType hostType);

    // Recompose a path from individual components into a file path string
    // for the current host. |components| is a vector os strings.
    // Returns a new file path string.
    template <class String>
    static std::string recompose(const std::vector<String>& components) {
        return PathUtils::recompose(components, HOST_TYPE);
    }

    // Given a list of components returned by decompose(), simplify it
    // by removing instances of '.' and '..' when that makes sense.
    // Note that it is not possible to simplify initial instances of
    // '..', i.e. "foo/../../bar" -> "../bar"
    static void simplifyComponents(std::vector<std::string_view>* components);
    static void simplifyComponents(std::vector<std::string>* components);
    template <class String>
    static void simplifyComponents(std::vector<String>* components);

    // Returns a version of |path| that is relative to |base|.
    // This can be useful for converting absolute paths to
    // relative paths given |base|.
    // Example:
    // |base|: C:\Users\foo
    // |path|: C:\Users\foo\AppData\Local\Android\Sdk
    // would give
    // AppData\Local\Android\Sdk.
    // If |base| is not a prefix of |path|, fails by returning
    // the original |path| unmodified.
    static std::string relativeTo(const std::string& base,
                                  const std::string& path,
                                  HostType hostType);
    static std::string relativeTo(const std::string& base,
                                  const std::string& path) {
        return relativeTo(base, path, HOST_TYPE);
    }

    static Optional<std::string> pathWithoutDirs(std::string_view name);
    static Optional<std::string> pathToDir(std::string_view name);

    // Replaces the entries ${xx} with the value of the environment variable
    // xx if it exists. Returns kNullopt if the environment variable is
    // not set or empty.
    static Optional<std::string> pathWithEnvSubstituted(std::string_view path);

    // Replaces the entries ${xx} with the value of the environment variable
    // xx if it exists. Returns kNullopt if the environment variable is
    // not set or empty.
    static Optional<std::string> pathWithEnvSubstituted(
            std::vector<std::string_view> decomposedPath);

    // Move a file. It works even when from and to are on different disks.
    static bool move(std::string_view from, std::string_view to);

#ifdef _WIN32
    static Win32UnicodeString asUnicodePath(std::string_view path) {
        return Win32UnicodeString(path);
    }
#else
    static CStrWrapper asUnicodePath(std::string_view path) {
        return c_str(path);
    }
#endif
};

// Useful shortcuts to avoid too much typing.
static const PathUtils::HostType kHostPosix = PathUtils::HOST_POSIX;
static const PathUtils::HostType kHostWin32 = PathUtils::HOST_WIN32;
static const PathUtils::HostType kHostType = PathUtils::HOST_TYPE;

template <class... Paths>
std::string pj(const std::string& path1,
               const std::string& path2,
               Paths&&... paths) {
    return PathUtils::join(path1,
                           pj(path2, std::forward<Paths>(paths)...));
}

std::string pj(const std::string& path1, const std::string& path2);

std::string pj(const std::vector<std::string>& paths);

}  // namespace base
}  // namespace android
