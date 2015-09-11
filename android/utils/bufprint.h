/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#ifndef ANDROID_UTILS_BUFPRINT_H
#define ANDROID_UTILS_BUFPRINT_H

#include "android/utils/compiler.h"

#include <stdarg.h>

ANDROID_BEGIN_HEADER

// WARNING: Usage of bufprint functions is considered deprecated. Try to
//          avoid them whenever possible.

/** FORMATTED BUFFER PRINTING
 **
 **  bufprint() allows your to easily and safely append formatted string
 **  content to a given bounded character buffer, in a way that is easier
 **  to use than raw snprintf()
 **
 **  'buffer'  is the start position in the buffer,
 **  'buffend' is the end of the buffer, the function assumes (buffer <= buffend)
 **  'format'  is a standard printf-style format string, followed by any number
 **            of formatting arguments
 **
 **  the function returns the next position in the buffer if everything fits
 **  in it. in case of overflow or formatting error, it will always return "buffend"
 **
 **  this allows you to chain several calls to bufprint() and only check for
 **  overflow at the end, for exemple:
 **
 **     char   buffer[1024];
 **     char*  p   = buffer;
 **     char*  end = p + sizeof(buffer);
 **
 **     p = bufprint(p, end, "%s/%s", first, second);
 **     p = bufprint(p, end, "/%s", third);
 **     if (p >= end) ---> overflow
 **
 **  as a convenience, the appended string is zero-terminated if there is no overflow.
 **  (this means that even if p >= end, the content of "buffer" is zero-terminated)
 **
 **  vbufprint() is a variant that accepts a va_list argument
 **/

extern char*   vbufprint(char*  buffer, char*  buffend, const char*  fmt, va_list  args );
extern char*   bufprint (char*  buffer, char*  buffend, const char*  fmt, ... );

// Append the application's directory to a bounded |buffer| that stops at
// |buffend|, and return the new position.
extern char*  bufprint_app_dir    (char*  buffer, char*  buffend);

// Append the root path containing all AVD sub-directories to a bounded
// |buffer| that stops at |buffend| and return new position. The default
// location can be overriden by defining ANDROID_AVD_HOME in the environment.
extern char*  bufprint_avd_home_path(char*  buffer, char*  buffend);

// Append the user-specific emulator configuration directory to a bounded
// |buffer| that stops at |buffend| and return the new position. The default
// location can be overriden by defining ANDROID_EMULATOR_HOME in the
// environment. Otherwise, a sub-directory of $HOME is used, unless
// ANDROID_SDK_HOME is also defined.
extern char*  bufprint_config_path(char*  buffer, char*  buffend);

// Append the name or a file |suffix| relative to the configuration
// directory (see bufprint_config_path) to the bounded |buffer| that stops
// at |buffend|, and return the new position.
extern char*  bufprint_config_file(char*  buffer, char*  buffend, const char*  suffix);

// Append the path to all emulator temporary files to a bounded |buffer| that
// stops at |buffend|, and return the new position.
extern char*  bufprint_temp_dir   (char*  buffer, char*  buffend);

// Append the path of a file or directory named |suffix| relative to the
// output of bufprint_temp_dir() to a bounded |buffer| that stops at
// |buffend|, and return the new position.
extern char*  bufprint_temp_file  (char*  buffer, char*  buffend, const char*  suffix);

ANDROID_END_HEADER

#endif  // ANDROID_UTILS_BUFPRINT_H
