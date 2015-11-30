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

#include <unistd.h>

#ifdef _WIN32

int safe_execv(const char* path, char* const* argv) {
   const int res = _spawnv(_P_WAIT, path, (const char* const*)argv);
   if (res == -1) {
       return -1;
   }

   exit(res);
}

#else

int safe_execv(const char* path, char* const* argv) {
    return execv(path, argv);
}

#endif
