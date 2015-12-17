#include "android/cpu_accelerator.h"

#include <string>

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

where argument is one of:
    -help       Show this help message
    -accel      Check the CPU acceleration support
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

    if (argv[1][0] != '/' && argv[1][0] != '-') {
        return error("Bad argument '%s'", argv[1]);
    }

    const std::string argument = argv[1] + 1;

    if (argument == "help") {
        return help();
    }
    if (argument == "accel") {
        return checkCpuAcceleration();
    }

    return error("Bad argument '%s'", argv[1]);
}
