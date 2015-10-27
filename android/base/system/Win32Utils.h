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

#include "android/base/Compiler.h"
#include "android/base/String.h"

#include <wchar.h>

#ifndef _WIN32
#error "Only include this file when compiling for Windows!"
#endif

namespace android {
namespace base {

// This class holds various utility functions for Windows host systems.
// All methods here must be static!
class Win32Utils {

public:
// Quote a given command-line command or parameter so that it can be
// sent to _execv() on Windows, and be received properly by the new
// process.
//
// This is necessary to work-around an annoying issue on Windows, where
// spawn() / exec() functions are mere wrappers around CreateProcess, which
// always pass the command-line as a _single_ string made of the simple
// concatenations of their arguments, while the new process will typically
// use CommandLineToArgv to convert it into a list of command-line arguments,
// expected the values to be quoted properly.
//
// For more details about this mess, read the MSDN blog post named
// "Everyone quotes arguments the wrong way".
//
// |commandLine| is an input string, that may contain spaces, quotes or
// backslashes. The function returns a new String that contains a version
// of |commandLine| that can be decoded properly by CommandLineToArgv().
static String quoteCommandLine(const char* commandLine);

// Helper class used to model a Windows Unicode string, which stores
// text as a zero-terminated array of UTF-16 code points. This is very
// intentionally _only_ used to perform conversions from/to String instances
// that hold the corresponding UTF-8 text. As such, you cannot assign, copy,
// append to them.
class UnicodeString {
public:
    // Default constructor.
    UnicodeString();

    // Initialize a new instance from UTF-8 text at |str| of |len| bytes.
    // This doesn't try to validate the input, i.e. on error, the instance's
    // content is undefined.
    UnicodeString(const char* str, size_t len);

    // Initialize a new instance from an existing String instance |str|.
    explicit UnicodeString(const String& str);

    // Initialize by reserving enough room for a string of |size| UTF-16
    // codepoints.
    explicit UnicodeString(size_t size);

    // Initialize from a zero-terminated wchar_t array.
    explicit UnicodeString(const wchar_t* str);

    // Destructor.
    ~UnicodeString();

    // Return pointer to first wchar_t in the string.
    const wchar_t* c_str() const { return mStr ? mStr : L""; }

    // Return pointer to writable wchar_t array. Only use with caution.
    wchar_t* data() { return mStr; }

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

    // Append at the end of a UnicodeString.
    void append(const wchar_t* other);
    void append(const wchar_t* other, size_t len);
    void append(const UnicodeString& other);

    // Release the Unicode string array to the caller.
    wchar_t* release();

private:
    wchar_t* mStr;
    size_t mSize;
};

};

}  // namespace base
}  // namespace android
