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

#ifndef ANDROID_BASE_STRING_FORMAT_H
#define ANDROID_BASE_STRING_FORMAT_H

#include "android/base/String.h"

#include <stdarg.h>

namespace android {
namespace base {

// Create a new String instance that contains the printf-style formatted
// output from |format| and potentially any following arguments.
String StringFormat(const char* format, ...);

// A variant of StringFormat() which uses a va_list to list formatting
// parameters instead.
String StringFormatWithArgs(const char* format, va_list args);

// Appends a formatted string at the end of an existing string.
// |string| is the target String instance, |format| the format string,
// followed by any formatting parameters. This is more efficient than
// appending the result of StringFormat(format,...) to |*string| directly.
void StringAppendFormat(String* string, const char* format, ...);

// A variant of StringAppendFormat() that takes a va_list to list
// formatting parameters.
void StringAppendFormatWithArgs(String* string,
                                const char* format,
                                va_list args);

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_STRING_FORMAT_H
