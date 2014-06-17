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

#include "android/utils/host_bitness.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

int android_getHostBitness(void) {
#ifdef _WIN32
    char directory[900];

    // Retrieves the path of the WOW64 system directory, which doesn't
    // exist on 32-bit systems.
    unsigned len = GetSystemWow64Directory(directory, sizeof(directory));
    if (len == 0) {
        return 32;
    } else {
        return 64;
    }
#else // !_WIN32
  /*
     This function returns 64 if host is running 64-bit OS, or 32 otherwise.

     It uses the same technique in ndk/build/core/ndk-common.sh.
     Here are comments from there:

  ## On Linux or Darwin, a 64-bit kernel (*) doesn't mean that the user-land
  ## is always 32-bit, so use "file" to determine the bitness of the shell
  ## that invoked us. The -L option is used to de-reference symlinks.
  ##
  ## Note that on Darwin, a single executable can contain both x86 and
  ## x86_64 machine code, so just look for x86_64 (darwin) or x86-64 (Linux)
  ## in the output.

    (*) ie. The following code doesn't always work:
        struct utsname u;
        int host_runs_64bit_OS = (uname(&u) == 0 && strcmp(u.machine, "x86_64") == 0);
  */
    return system("file -L \"$SHELL\" | grep -q \"x86[_-]64\"") == 0 ? 64 : 32;
#endif // !_WIN32
}
