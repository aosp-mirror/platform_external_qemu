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

#include "android/wear-agent/WearAgent.h"

#include "android/base/Limits.h"
extern "C" {
#include "android/looper.h"
#include "android/sockets.h"
}

#include <stdio.h>


extern void print_help(const char* progname);

int main(int argc, char** argv)
{
    if (argc > 1) {
        print_help(argv[0]);
        return 0;
    }
#ifdef _WIN32
    socket_init();
#endif

    Looper* mainLooper = looper_newGeneric();

    android::wear::WearAgent agent(mainLooper);
    
    looper_run(mainLooper);

    return 0;
}


