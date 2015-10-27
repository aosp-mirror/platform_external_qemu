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

#ifndef _WIN32
#error "This header should only be included when building Windows binaries!"
#endif

#include "android/base/Compiler.h"
#include "android/base/String.h"

#include <wchar.h>

namespace android {
namespace base {

// Helper class used to model a Windows Unicode string, which stores
// text and file paths as a zero-terminated array of UTF-16 code points
// (at least since Windows Vista, before that the encoding was slightly
// different).
//
// This is very intentionally *not* a general purpose class. It should only
// be used to simplify conversions between the Win32 Unicode API and the
// rest of android::base which uses UTF-8 for all Unicode text.
class Win32UnicodeString {
public:
    // Default constructor.
    Win32UnicodeString();

    // Initialize a new instance from UTF-8 text at |str| of |len| bytes.
    // This doesn't try to validate the input, i.e. on error, the instance's
    // content is undefined.
    Win32UnicodeString(const char* str, size_t len);

    // Initialize a new instance from an existing String instance |str|.
    explicit Win32UnicodeString(const String& str);

    // Initialize by reserving enough room for a string of |size| UTF-16
    // codepoints.
    explicit Win32UnicodeString(size_t size);

    // Initialize from a zero-terminated wchar_t array.
    explicit Win32UnicodeString(const wchar_t* str);

    // Destructor.
    ~Win32UnicodeString();

    // Return pointer to first wchar_t in the string.
    const wchar_t* c_str() const { return mStr ? mStr : L""; }

    // Return pointer to writable wchar_t array. This can never be NULL
    // but no more than size() items should be accessed.
    wchar_t* data();

    // Return size of the string, this is the number of UTF-16 code points
    // stored by the string, which may be larger than the number of actual
    // Unicode characters in it.
    size_t size() const { return mSize; }

    // Convert to a String instance holding the corresponding UTF-8 text.
    String toString() const;

    // Reset content from UTF-8 text at |str| or |len| bytes.
    void reset(const char* str, size_t len);

    // Reset content from UTF-8 text at |str|.
    void reset(const String& str);

    // Resize array.
    void resize(size_t newSize);

    // Append at the end of a Win32UnicodeString.
    void append(const wchar_t* other);
    void append(const wchar_t* other, size_t len);
    void append(const Win32UnicodeString& other);

    // Release the Unicode string array to the caller.
    wchar_t* release();

    // Static methods that directly convert a Unicode string to UTF-8 text.
    static String convertToUtf8(const wchar_t* str);
    static String convertToUtf8(const wchar_t* str, size_t len);

private:
    DISALLOW_COPY_AND_ASSIGN(Win32UnicodeString);

    wchar_t* mStr;
    size_t mSize;
};

}  // namespace base
}  // namespace android
