// Copyright 2014-2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// This source file implements emulator-arm64 and emulator64-arm64
// which are used to launch QEMU binaries located under
// $PROGRAM_DIR/qemu/<host>/qemu-system-aarch64<exe>

#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"

#include "android/help.h"
#include "android/utils/stralloc.h"
#include "android/version.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using android::base::String;
using android::base::StringVector;
using android::base::StringFormat;
using android::base::PathUtils;

// The host CPU architecture.
#ifdef __i386__
static const char kHostArch[] = "x86";
#elif defined(__x86_64__)
static const char kHostArch[] = "x86_64";
#else
#error "Your host CPU is not supported!"
#endif

// The host operating system name.
#ifdef __linux__
static const char kHostOs[] = "linux";
#elif defined(__APPLE__)
static const char kHostOs[] = "darwin";
#elif defined(_WIN32)
static const char kHostOs[] = "windows";
#endif

static const char kQemuArch[] =
#ifdef TARGET_ARM64
    "aarch64"
#elif defined(TARGET_MIPS64)
    "mips64el"
#elif defined(TARGET_MIPS)
    "mipsel"
#elif defined(TARGET_X86)
    "i386"
#elif defined(TARGET_X86_64)
    "x86_64"
#else
    #error "No target architecture macro. Bad makefile?"
#endif
        ;

/* The execv() definition in older mingw is slightly bogus.
 * It takes a second argument of type 'const char* const*'
 * while POSIX mandates char** instead.
 *
 * To avoid compiler warnings, define the safe_execv macro
 * to perform an explicit cast with mingw.
 */
#if defined(_WIN32) && !ANDROID_GCC_PREREQ(4,4)
#  define safe_execv(_filepath,_argv)  execv((_filepath),(const char* const*)(_argv))
#else
#  define safe_execv(_filepath,_argv)  execv((_filepath),(_argv))
#endif

static void emulator_help() {
    STRALLOC_DEFINE(out);
    android_help_main(out);
    printf("%.*s", out->n, out->s);
    stralloc_reset(out);
    exit(1);
}

static void emulator_version() {
    printf("Android emulator version %s\n"
           "Copyright (C) 2006-2015 The Android Open Source Project and many others.\n"
           "This program is a derivative of the QEMU CPU emulator (www.qemu.org).\n\n",
#if defined ANDROID_BUILD_ID
           VERSION_STRING " (build_id " STRINGIFY(ANDROID_BUILD_ID) ")" );
#else
           VERSION_STRING);
#endif
    printf("  This software is licensed under the terms of the GNU General Public\n"
           "  License version 2, as published by the Free Software Foundation, and\n"
           "  may be copied, distributed, and modified under those terms.\n\n"
           "  This program is distributed in the hope that it will be useful,\n"
           "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
           "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
           "  GNU General Public License for more details.\n\n");

    exit(0);
}

// Return the path of the QEMU executable
static String getQemuExecutablePath(const char* programPath) {
    StringVector path = PathUtils::decompose(programPath);
    if (path.size() < 1) {
        return String();
    }
    // Remove program from path.
    path.resize(path.size() - 1U);

    // Add sub-directories.
    path.append(String("qemu"));

    String host = kHostOs;
    host += "-";
    host += kHostArch;
    path.append(host);

    String qemuProgram = "qemu-system-";
    qemuProgram += kQemuArch;
#ifdef _WIN32
    qemuProgram += ".exe";
#endif
    path.append(qemuProgram);

    return PathUtils::recompose(path);
}

extern "C" int main(int argc, char** argv) {
    if (argc < 1) {
        fprintf(stderr, "Invalid invocation (no program path)\n");
        return 1;
    }

    String qemuExecutable = getQemuExecutablePath(argv[0]);
    const char* args[128] = {};

    int argIdx = 1;

    while (argc-- > 1) {
        const char* opt = (++argv)[0];

        if(!strcmp(opt, "-version")) {
            emulator_version(); // doesn't return
        }
        if (!strcmp(opt, "-help")) {
            emulator_help(); // doesn't return
        }
        if (!strncmp(opt, "-help-",6)) {
            STRALLOC_DEFINE(out);
            opt += 6;

            if (!strcmp(opt, "all")) {
                android_help_all(out);
            }
            else if (android_help_for_option(opt, out) == 0) {
                /* ok */
            }
            else if (android_help_for_topic(opt, out) == 0) {
                /* ok */
            }
            if (out->n > 0) {
                printf("\n%.*s", out->n, out->s);
                exit(0);
            }

            fprintf(stderr, "unknown option: -help-%s\n", opt);
            fprintf(stderr, "please use -help for a list of valid topics\n");
            exit(1);
        }

        assert(argIdx < (int)(sizeof(args)/sizeof(args[0])));
        args[argIdx++] = opt;
    }

    args[0] = qemuExecutable.c_str();
    safe_execv(args[0], (char* const*)args);

    fprintf(stderr,
            "Could not launch QEMU [%s]: %s\n",
            args[0],
            strerror(errno));

    return errno;
}
