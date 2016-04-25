// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/exec.h"

#include "android/base/system/Win32UnicodeString.h"

#include <unistd.h>

#ifdef _WIN32
#include <vector>
#endif

#ifdef _WIN32

#include <windows.h>

static HANDLE handle = nullptr;
static BOOL WINAPI ctrl_handler(DWORD type)
{
    fflush(stdout);
    fflush(stderr);

    if (!handle) {
        return TRUE;
    }

    /* Windows 7 kills application when the function returns.
       Sleep here to give QEMU a try for closing.
       Windows also kills the program after 10 seconds anyway. */
    if (::WaitForSingleObject(handle, 9000) != WAIT_OBJECT_0) {
        ::TerminateProcess(handle, 100);
    }
    exit(1);

    return TRUE;
}

using android::base::Win32UnicodeString;

int safe_execv(const char* path, char* const* argv) {
   std::vector<Win32UnicodeString> arguments;
   for (size_t i = 0; argv[i] != nullptr; ++i) {
      arguments.push_back(Win32UnicodeString(argv[i]));
   }
   // Do this in two steps since the pointers might change because of push_back
   // in the loop above.
   std::vector<const wchar_t*> argumentPointers;
   for (const auto& arg : arguments) {
      argumentPointers.push_back(arg.c_str());
   }
   argumentPointers.push_back(nullptr);
   Win32UnicodeString program(path);

   SetConsoleCtrlHandler(ctrl_handler, TRUE);

   handle = (HANDLE)_wspawnv(_P_NOWAIT, program.c_str(), &argumentPointers[0]);
   if (handle == nullptr) {
       return 1;
   }
   ::WaitForSingleObject(handle, INFINITE);
   DWORD exitCode;
   if (!::GetExitCodeProcess(handle, &exitCode)) {
       exitCode = 2;
   }
   exit(exitCode);
}

#else

int safe_execv(const char* path, char* const* argv) {
    return execv(path, argv);
}

#endif
