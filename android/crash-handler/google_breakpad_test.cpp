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

#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    int ret = android_setupGoogleBreakpad("/tmp");
    if (ret < 0) {
        fprintf(stderr, "Cannot setup Breakpad crash handler: %s\n",
                strerror(-ret));
        return -ret;
    }
    printf("OK: Breakpad handler registered!\n");
    *(int*)0 = 0xdeadbeef;
    return 0;
}