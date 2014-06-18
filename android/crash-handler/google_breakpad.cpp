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

#include "android/crash-handler/google_breakpad.h"

#if defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#elif defined(_WIN32)
#include "client/windows/handler/exception_handler.h"
#elif defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#else
#error "Your system is not supported by Breakpad!"
#endif

#include "android/utils/assert.h"
#include "android/utils/path.h"

using namespace google_breakpad;

namespace {

// Global and leaky exception handler instance. Allocated on demand.
google_breakpad::ExceptionHandler* sExceptionHandler;


#if defined(__linux__)
bool dumpCallback(const MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
    printf("Crash report written to: %s\n", descriptor.path());
    return succeeded;
}
#endif  // __linux__

#if defined(_WIN32)
bool dumpCallback(const wchar_t* dump_path,
                  const wchar_t* minidump_id,
                  void* context,
                  EXCEPTION_POINTERS* exinfo,
                  MDRawAssertionInfo* assertion,
                  bool succeeded) {
    wprintf(L"Crash report written to: %ls/%ls\n", dump_path, minidump_id);
    return succeeded;
}
#endif  // _WIN32

#if defined(__APPLE__)
bool dumpCallback(const char* dump_dir,
                  const char* minidump_id,
                  void* context,
                  bool succeeded) {
    printf("Crash report written to: %s/%s\n", dump_dir, minidump_id);
    return succeeded;
}
#endif  // __APPLE__

}  // namespace


int android_setupGoogleBreakpad(const char* minidumps_dir) {
    // Create directory if needed.
    if (!path_exists(minidumps_dir)) {
        if (path_mkdir_if_needed(minidumps_dir, 0666) < 0) {
            return -errno;
        }
    }

#ifdef __linux__
    MinidumpDescriptor descriptor(minidumps_dir);

    sExceptionHandler = new ExceptionHandler(
            MinidumpDescriptor(minidumps_dir),
            NULL,
            dumpCallback,
            NULL,
            true,
            -1);

#elif defined(_WIN32)
    // convert path to wstring.
    std::wstring dump_dir;
    const char* s = minidumps_dir;
    size_t len = mbsrtowcs(NULL, &s, 0, NULL);
    if (len == (size_t)-1) {
        return -EINVAL;
    }
    dump_dir.resize(len);
    s = minidumps_dir;
    mbsrtowcs(&dump_dir[0], &s, len, NULL);

    sExceptionHandler = new ExceptionHandler(
            dump_dir,
            NULL,  // no filter
            dumpCallback,  // callback
            NULL,  // no calback contetxt
            ExceptionHandler::HANDLER_ALL);

#elif defined(__APPLE__)
    sExceptionHandler = new google_breakpad::ExceptionHandler(
            std::string(minidumps_dir),
            NULL,  // no filter
            dumpCallback,  // callback
            NULL,  // no callback opaque
            true,  // install handler
            NULL   // no out-of-process crash generation
            );
#else
#error "Your host system is not supported!"
#endif

    return 0;
}
