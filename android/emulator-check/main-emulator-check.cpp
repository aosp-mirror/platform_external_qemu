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
#include "android/emulation/CpuAccelerator.h"
#include "android/emulator-check/PlatformInfo.h"

#include <tuple>

#include <stdio.h>
#include <stdlib.h>

static const int kGenericError = 100;

static int help();

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

static int checkHyperV() {
    AndroidHyperVStatus status;
    std::string message;
    std::tie(status, message) = android::GetHyperVStatus();
    printf("%s\n", message.c_str());
    return status;
}

static int getCpuInfo() {
    AndroidCpuInfoFlags flags;
    std::string message;
    std::tie(flags, message) = android::GetCpuInfo();
    printf("%s\n", message.c_str());
    return flags;
}

static int printWindowManagerName() {
    const auto name = android::getWindowManagerName();
    printf("%s\n", name.c_str());
    return name.empty() ? kGenericError : 0;
}

constexpr struct {
    const char* arg;
    const char* help;
    int (* handler)();
} options[] = {
{
    "-h",
    "Show this help message",
    &help
},
{
    "-help",
    "Show this help message",
    &help
},
{
    "--help",
    "Show this help message",
    &help
},
{
    "accel",
    "Check the CPU acceleration support",
    &checkCpuAcceleration
},
{
    "hyper-v",
    "Check if hyper-v is installed and running (Windows)",
    &checkHyperV
},
{
    "cpu-info",
    "Return the CPU model information",
    &getCpuInfo
},
{
    "window-mgr",
    "Return the current window manager name",
    &printWindowManagerName
},
};

static void usage() {
    printf("%s\n",
R"(Usage: emulator-check <argument>

Performs the check requested in <argument> and returns the result by the means
of return code and text message

<argument> is one of:
)");

    for (const auto& option : options) {
        printf("    %s\t\t%s\n", option.arg, option.help);
    }

    printf("\n");
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
    return kGenericError;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        return error("Missing a required argument");
    }

    const android::base::StringView argument = argv[1];

    for (const auto& option : options) {
        if (argument == option.arg) {
            return option.handler();
        }
    }

    return error("Bad argument '%s'", argv[1]);
}
