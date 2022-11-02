// Copyright 2022 The Android Open Source Project
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
#include "android/utils/compiler.h"
#include <stdarg.h>


#ifdef __cplusplus
#include <utility>
#include <stdio.h>
#include "aemu/base/StringFormat.h"
#else
#include <stdbool.h>
#endif


#if defined(__cplusplus) && ! defined(__clang__)
#define ANDROID_NORETURN [[noreturn]]
#else  // !__cplusplus || __clang__
#ifdef _MSC_VER
#define ANDROID_NORETURN __declspec(noreturn)
#else  // !_MSC_VER
#define ANDROID_NORETURN __attribute__((noreturn))
#endif  // !_MSC_VER
#endif  // !__cplusplus || __clang__


ANDROID_BEGIN_HEADER

// Call this to enable appending of messages.
void crashhandler_enable_message_store();

// Append message to crash dump file without aborting.
// Useful for saving debugging information just before a crash.
//
// Note, this message will be added to a crash attrbute, it is not
// guaranteed to make it to the final report as we only have 
// limited amount of storage space available.
void crashhandler_append_message(const char* message);

// A variadic overload with C interface
void crashhandler_append_message_format(const char* format, ...);
void crashhandler_append_message_format_v(const char* format, va_list args);

// Append a string to the crash report. One is free to free both of the
// parameters after the call - function doesn't take ownership of them.
//
// The strings are stored in the minidump as 'name' : 'string'
void crashhandler_add_string(const char* name, const char* string);

// Reads the contents of file |source| and attaching it as a string annotation 
// under |source|. This is the same as reading the file source and calling
// crashhandler_add_string(destination, contents_of_file_source)
bool crashhandler_copy_attachment(const char* destination, const char* source);



// Track crashes on exit.
void crashhandler_exitmode(const char* message);

// Abort the program execution immediately, recording the message
// with the crashreport if possible.
ANDROID_NORETURN void crashhandler_die(const char* message)
    __attribute__((noinline));

// A variadic overload with C interface
ANDROID_NORETURN void crashhandler_die_format(const char* format, ...);
ANDROID_NORETURN void crashhandler_die_format_v(const char* format, va_list args);

void pause_hangdetector();
void resume_hangdetector();
void detect_hanging_looper(void*);
ANDROID_END_HEADER


#ifdef __cplusplus
// A variadic overload for a convenient message formatting
template <class... Args>
ANDROID_NORETURN void crashhandler_die(const char* format, Args&&... args) {
    char buffer[2048] = {};    // 2048 is enough for everyone ;)
    snprintf(buffer, sizeof(buffer) - 1, format,
             android::base::unpackFormatArg(std::forward<Args>(args))...);
    crashhandler_die(buffer);
}
#endif