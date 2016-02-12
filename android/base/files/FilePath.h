// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/StringView.h"

#include <string>

#ifdef _WIN32
#define PATH_LITERAL(x) L ## x
// Macro for use in printf-like format strings where the parameter is going
// to be a FilePath::c_str()
#define PRIfp "S"
#else
#define PATH_LITERAL(x) x
// Macro for use in printf-like format strings where the parameter is going
// to be a FilePath::c_str()
#define PRIfp "s"
#endif

namespace android {
namespace base {

// A class that contains a path to a file. This path is represented in the
// format that matches the current platform. In practice this means that it's
// a std::wstring on Windows and std::string on Linux and Darwin.
//
// Apart from containing a path to a file it also allows some basic operations
// on file paths such as appending and retrieving parts of the file path.
//
// When composing paths using raw string literals please use the PATH_LITERAL
// macro defined above as this will ensure that the raw string literal has a
// type that matches the platform. This also avoids unnecessary conversion
// between encodings.
class FilePath {
public:
#ifdef _WIN32
    using StringType = std::wstring;
#else
    using StringType = std::string;
#endif
    using CharType = StringType::value_type;

    // A list of acceptable path separator for this platform, the separator
    // at [0] is the preferred one for this platform
    static CharType kSeparators[];

    // Create a FilePath representing |path|. UTF-8 encoding will be assumed
    // and any necessary conversion to the platform encoding will be done
    FilePath(StringView path);
    FilePath(const String& path);

    // Construct a FilePath from a natively encoded |path|.
    FilePath(const CharType* path) : mPath(path) { }
    FilePath(const StringType& path) : mPath(path) { }

#ifdef _WIN32
    // On non-char platforms allow explicit conversion from UTF-8 strings
    explicit FilePath(const char* path);
    explicit FilePath(const std::string& path);
#endif

    // Copy constructor
    FilePath(const FilePath& other) : mPath(other.mPath) { }

    // Assignment operator
    FilePath& operator=(const FilePath& other) {
        mPath = other.mPath;
        return *this;
    }

    // Join two path components together, separating them by a single path
    // separator. Returns the resulting path.
    FilePath operator+(const FilePath& other) const;

    // Get the path as a string in the platform representation
    const StringType& get() const { return mPath; }

    // Get a raw string representing the path in platform encoding
    const CharType* c_str() const { return mPath.c_str(); }

    // Get the number of characters in the path
    size_t size() const { return mPath.size(); }

    // Get the path encoded as UTF-8, may require conversion
    String asUtf8() const;

    // Determine if a given character is a path separator on this platform
    bool isPathSeparator(CharType c) const;
private:
    StringType mPath;
};

inline FilePath FilePath::operator+(const FilePath& other) const {
    FilePath result(*this);

    while (result.size() > 0 &&
           isPathSeparator(result.mPath[result.mPath.size() - 1])) {
        result.mPath.resize(result.mPath.size() - 1);
    }

    result.mPath += kSeparators[0];
    result.mPath += other.get();
    return result;
}

}  // namespace base
}  // namespace android
