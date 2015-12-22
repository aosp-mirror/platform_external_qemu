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

#include "android/base/StringView.h"
#include "android/cpu_accelerator.h"

#include <stdlib.h>
#include <stdio.h>

// check ability to launch haxm/kvm accelerated VM and exit
// designed for use by Android Studio
static int checkCpuAcceleration() {
    char* status = nullptr;
    AndroidCpuAcceleration capability =
            androidCpuAcceleration_getStatus(&status);
    if (status) {
        FILE* out = stderr;
        if (capability == ANDROID_CPU_ACCELERATION_READY)
            out = stdout;
        fprintf(out, "%s\n", status);
        free(status);
    }
    return capability;
}


static void usage() {
    printf("%s\n",
R"(Usage: emulator-check <argument>

Performs the check requested in <argument> and returns the result by the means
of return code and text message

Argument is one of:
    accel       Check the CPU acceleration support
    -help       Show this help message
)");
}

static int help() {
    usage();
    return 0;
}

static int error(const char* format, const char* arg = nullptr) {
    if (format) {
        fprintf(stderr, format, arg);
        fprintf(stderr, "\n\n");
    }
    usage();
    return 100;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        return error("Missing a required argument");
    }

    const android::base::StringView argument = argv[1];

    if (argument == "accel") {
        return checkCpuAcceleration();
    }
    if (argument == "--help" || argument == "-help" || argument == "-h") {
        return help();
    }

    return error("Bad argument '%s'", argv[1]);
}
