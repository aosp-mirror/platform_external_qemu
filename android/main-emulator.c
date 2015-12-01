/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/* This is the source code to the tiny "emulator" launcher program
 * that is in charge of starting the target-specific emulator binary
 * for a given AVD, i.e. either 'emulator-arm' or 'emulator-x86'
 *
 * This program will be replaced in the future by what is currently
 * known as 'emulator-ui', but is a good placeholder until this
 * migration is completed.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "android/avd/scanner.h"
#include "android/avd/util.h"
#include "android/cpu_accelerator.h"
#include "android/opengl/emugl_config.h"
#include "android/qt/qt_setup.h"
#include "android/utils/compiler.h"
#include "android/utils/exec.h"
#include "android/utils/host_bitness.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"
#include "android/utils/bufprint.h"
#include "android/utils/win32_cmdline_quote.h"


/* Required by android/utils/debug.h */
int android_verbose;
bool ranchu = false;

#define DEBUG 1

#if DEBUG
#  define D(...)  do { if (android_verbose) printf("emulator:" __VA_ARGS__); } while (0)
#else
#  define D(...)  do{}while(0)
#endif

/* The extension used by dynamic libraries on the host platform */
#ifdef _WIN32
#  define DLL_EXTENSION  ".dll"
#elif defined(__APPLE__)
#  define DLL_EXTENSION  ".dylib"
#else
#  define DLL_EXTENSION  ".so"
#endif

// Name of GPU emulation main library for (32-bit and 64-bit versions)
#define GLES_EMULATION_LIB    "libOpenglRender" DLL_EXTENSION
#define GLES_EMULATION_LIB64  "lib64OpenglRender" DLL_EXTENSION

/* Forward declarations */
static char* getTargetEmulatorPath(const char* progDir,
                                   bool tryCurrentPath,
                                   const char* avdArch,
                                   const int force_32bit,
                                   bool* is_64bit);

static void updateLibrarySearchPath(bool is_64bit);

/* Main routine */
int main(int argc, char** argv)
{
    const char* avdName = NULL;
    char*       avdArch = NULL;
    const char* gpu = NULL;
    char*       emulatorPath;
    int         force_32bit = 0;
    bool        no_window = false;

    /* Define ANDROID_EMULATOR_DEBUG to 1 in your environment if you want to
     * see the debug messages from this launcher program.
     */
    const char* debug = getenv("ANDROID_EMULATOR_DEBUG");

    if (debug != NULL && *debug && *debug != '0')
        android_verbose = 1;

    /* Parse command-line and look for
     * 1) an avd name either in the form or '-avd <name>' or '@<name>'
     * 2) '-force-32bit' which always use 32-bit emulator on 64-bit platforms
     * 3) '-verbose', or '-debug-all' or '-debug all' to enable verbose mode.
     */
    int  nn;
    for (nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];

        if (!strcmp(opt, "-accel-check")) {
            // check ability to launch haxm/kvm accelerated VM and exit
            // designed for use by Android Studio
            char* status = 0;
            AndroidCpuAcceleration capability =
                    androidCpuAcceleration_getStatus(&status);
            if (status) {
                FILE* out = stderr;
                if (capability == ANDROID_CPU_ACCELERATION_READY)
                    out = stdout;
                fprintf(out, "%s\n", status);
                free(status);
            }
            exit(capability);
        }

        if (!strcmp(opt,"-qemu"))
            break;

        if (!strcmp(opt,"-verbose") || !strcmp(opt,"-debug-all")) {
            android_verbose = 1;
        }

        if (!strcmp(opt,"-debug") && nn + 1 < argc &&
            !strcmp(argv[nn + 1], "all")) {
            android_verbose = 1;
        }

        if (!strcmp(opt,"-gpu") && nn + 1 < argc) {
            gpu = argv[nn + 1];
            nn++;
        }

        if (!strcmp(opt,"-ranchu")) {
            ranchu = true;
            continue;
        }

        if (!strcmp(opt,"-force-32bit")) {
            force_32bit = 1;
            continue;
        }

        if (!strcmp(opt,"-no-window")) {
            no_window = true;
            continue;
        }

        if (!strcmp(opt,"-list-avds")) {
            AvdScanner* scanner = avdScanner_new(NULL);
            for (;;) {
                const char* name = avdScanner_next(scanner);
                if (!name) {
                    break;
                }
                printf("%s\n", name);
            }
            avdScanner_free(scanner);
            exit(0);
        }

        if (!avdName) {
            if (!strcmp(opt,"-avd") && nn+1 < argc) {
                avdName = argv[nn+1];
            }
            else if (opt[0] == '@' && opt[1] != '\0') {
                avdName = opt+1;
            }
        }
    }

    /* If ANDROID_EMULATOR_FORCE_32BIT is set to 'true' or '1' in the
     * environment, set -force-32bit automatically.
     */
    {
        const char kEnvVar[] = "ANDROID_EMULATOR_FORCE_32BIT";
        const char* val = getenv(kEnvVar);
        if (val && (!strcmp(val, "true") || !strcmp(val, "1"))) {
            if (!force_32bit) {
                D("Auto-config: -force-32bit (%s=%s)\n", kEnvVar, val);
                force_32bit = 1;
            }
        }
    }

#if defined(__linux__)
    if (!force_32bit && android_getHostBitness() == 32) {
        fprintf(stderr,
"ERROR: 32-bit Linux Android emulator binaries are DEPRECATED, to use them\n"
"       you will have to do at least one of the following:\n"
"\n"
"       - Use the '-force-32bit' option when invoking 'emulator'.\n"
"       - Set ANDROID_EMULATOR_FORCE_32BIT to 'true' in your environment.\n"
"\n"
"       Either one will allow you to use the 32-bit binaries, but please be\n"
"       aware that these will disappear in a future Android SDK release.\n"
"       Consider moving to a 64-bit Linux system before that happens.\n"
"\n"
        );
        exit(1);
    }
#elif defined(_WIN32)
    // Windows version of Qemu1 works only in x86 mode
    force_32bit = !ranchu;
#endif

    /* If there is an AVD name, we're going to extract its target architecture
     * by looking at its config.ini
     */
    if (avdName != NULL) {
        D("Found AVD name '%s'\n", avdName);
        avdArch = path_getAvdTargetArch(avdName);
        D("Found AVD target architecture: %s\n", avdArch);
    } else {
        /* Otherwise, using the ANDROID_PRODUCT_OUT directory */
        const char* androidOut = getenv("ANDROID_PRODUCT_OUT");

        if (androidOut != NULL) {
            D("Found ANDROID_PRODUCT_OUT: %s\n", androidOut);
            avdArch = path_getBuildTargetArch(androidOut);
            D("Found build target architecture: %s\n",
              avdArch ? avdArch : "<NULL>");
        }
    }

    if (avdArch == NULL) {
        avdArch = "arm";
        D("Can't determine target AVD architecture: defaulting to %s\n", avdArch);
    }

    /* Find program directory. */
    char* progDir = NULL;
    path_split(argv[0], &progDir, NULL);

    /* Only search in current path if there is no directory separator
     * in |progName|. */
#ifdef _WIN32
    bool tryCurrentPath = (!strchr(argv[0], '/') && !strchr(argv[0], '\\'));
#else
    bool tryCurrentPath = !strchr(argv[0], '/');
#endif

    /* Find the architecture-specific program in the same directory */
    bool is_64bit = false;
    emulatorPath = getTargetEmulatorPath(progDir,
                                         tryCurrentPath,
                                         avdArch,
                                         force_32bit,
                                         &is_64bit);
    D("Found target-specific emulator binary: %s\n", emulatorPath);

    /* Replace it in our command-line */
    argv[0] = emulatorPath;

    /* Setup library paths so that bundled standard shared libraries are picked
     * up by the re-exec'ed emulator
     */
    updateLibrarySearchPath(is_64bit);

    /* We need to find the location of the GLES emulation shared libraries
     * and modify either LD_LIBRARY_PATH or PATH accordingly
     */
    bool gpuEnabled = false;
    const char* gpuMode = NULL;

    if (avdName) {
        gpuMode = path_getAvdGpuMode(avdName);
        gpuEnabled = (gpuMode != NULL);
    }

    EmuglConfig config;
    int bitness = is_64bit ? 64 : 32;
    if (!emuglConfig_init(
                &config, gpuEnabled, gpuMode, gpu, bitness, no_window)) {
        fprintf(stderr, "ERROR: %s\n", config.status);
        exit(1);
    }
    D("%s\n", config.status);

    emuglConfig_setupEnv(&config);

    /* Add <lib>/qt/ to the library search path. */
    androidQtSetupEnv(bitness);

#ifdef _WIN32
    // Take care of quoting all parameters before sending them to execv().
    // See the "Eveyone quotes command line arguments the wrong way" on
    // MSDN.
    int n;
    for (n = 0; n < argc; ++n) {
        // Technically, this leaks the quoted strings, but we don't care
        // since this process will terminate after the execv() anyway.
        argv[n] = win32_cmdline_quote(argv[n]);
        D("Quoted param: [%s]\n", argv[n]);
    }
#endif

    if (android_verbose) {
        int i;
        printf("emulator: Running :%s\n", emulatorPath);
        for(i = 0; i < argc; i++) {
            printf("emulator: qemu backend: argv[%02d] = \"%s\"\n", i, argv[i]);
        }
        /* Dump final command-line parameters to make debugging easier */
        printf("emulator: Concatenated backend parameters:\n");
        for (i = 0; i < argc; i++) {
            /* To make it easier to copy-paste the output to a command-line,
             * quote anything that contains spaces.
             */
            if (strchr(argv[i], ' ') != NULL) {
                printf(" '%s'", argv[i]);
            } else {
                printf(" %s", argv[i]);
            }
        }
        printf("\n");
    }

    // Launch it with the same set of options !
    // Note that on Windows, the first argument must _not_ be quoted or
    // Windows will fail to find the program.
    safe_execv(emulatorPath, argv);

    /* We could not launch the program ! */
    fprintf(stderr, "Could not launch '%s': %s\n", emulatorPath, strerror(errno));
    return errno;
}

static char* bufprint_emulatorName(char* p,
                                   char* end,
                                   const char* progDir,
                                   const char* prefix,
                                   const char* variant,
                                   const char* archSuffix,
                                   const char* exeExtension) {
    if (progDir) {
        p = bufprint(p, end, "%s/", progDir);
    }
    p = bufprint(p, end, "%s", prefix);
    if (variant) {
        p = bufprint(p, end, "%s-", variant);
    }
    p = bufprint(p, end, "%s%s", archSuffix, exeExtension);
    return p;
}

/* Probe the filesystem to check if an emulator executable named like
 * <progDir>/<prefix><arch> exists.
 *
 * |progDir| is an optional program directory. If NULL, the executable
 * will be searched in the current directory.
 * |variant| is an optional variant. If not NULL, then a name like
 * 'emulator-<variant>-<archSuffix>' will be searched, instead of
 * 'emulator-<archSuffix>'.
 * |archSuffix| is an architecture-specific suffix, like "arm", or 'x86"
 * If |search_for_64bit_emulator| is true, lookup for 64-bit emulator first,
 * then the 32-bit version.
 * If |try_current_path|, try to look into the current path if no
 * executable was found under |progDir|.
 * On success, returns the path of the executable (string must be freed by
 * the caller) and sets |*is_64bit| to indicate whether the binary is a
 * 64-bit executable. On failure, return NULL.
 */
static char*
probeTargetEmulatorPath(const char* progDir,
                        const char* variant,
                        const char* archSuffix,
                        bool search_for_64bit_emulator,
                        bool try_current_path,
                        bool* is_64bit)
{
    char path[PATH_MAX], *pathEnd = path + sizeof(path), *p;

    static const char kEmulatorPrefix[] = "emulator-";
    static const char kEmulator64Prefix[] = "emulator64-";
#ifdef _WIN32
    const char kExeExtension[] = ".exe";
#else
    const char kExeExtension[] = "";
#endif

    // First search for the 64-bit emulator binary.
    if (search_for_64bit_emulator) {
        p = bufprint_emulatorName(path,
                                  pathEnd,
                                  progDir,
                                  kEmulator64Prefix,
                                  variant,
                                  archSuffix,
                                  kExeExtension);
        D("Probing program: %s\n", path);
        if (p < pathEnd && path_exists(path)) {
            *is_64bit = true;
            return strdup(path);
        }
    }

    // Then for the 32-bit one.
    p = bufprint_emulatorName(path,
                                pathEnd,
                                progDir,
                                kEmulatorPrefix,
                                variant,
                                archSuffix,
                                kExeExtension);
    D("Probing program: %s\n", path);
    if (p < pathEnd && path_exists(path)) {
        *is_64bit = false;
        return strdup(path);
    }

    // Not found, try in the current path then
    if (try_current_path) {
        char* result;

        if (search_for_64bit_emulator) {
            p = bufprint_emulatorName(path,
                                      pathEnd,
                                      NULL,
                                      kEmulator64Prefix,
                                      variant,
                                      archSuffix,
                                      kExeExtension);
            if (p < pathEnd) {
                D("Probing path for: %s\n", path);
                result = path_search_exec(path);
                if (result) {
                    *is_64bit = true;
                    return result;
                }
            }
        }

        p = bufprint_emulatorName(path,
                                    pathEnd,
                                    NULL,
                                    kEmulatorPrefix,
                                    variant,
                                    archSuffix,
                                    kExeExtension);
        if (p < pathEnd) {
            D("Probing path for: %s\n", path);
            result = path_search_exec(path);
            if (result) {
                *is_64bit = false;
                return result;
            }
        }
    }

    return NULL;
}

/*
 * emulator engine selection process is
 * emulator ->
 *   arm:
 *     32-bit: emulator(64)-arm
 *     64-bit: emulator(64)-arm64(NA) ->
 *             emulator(64)-ranchu-arm64 -> qemu/<host>/qemu-system-aarch64
 *   mips:
 *     32-bit: (if '-ranchu')
 *               emulator(64)-ranchu-mips -> qemu/<host>/qemu-system-mipsel
 *             else
 *               emulator(64)-mips
 *     64-bit: emulator(64)-mips64(NA) ->
 *             emulator(64)-ranchu-mips64 -> qemu/<host>/qemu-system-mips64
 *   x86:
 *     32-bit: (if '-ranchu')
 *               emulator(64)-ranchu-x86 -> qemu/<host>/qemu-system-x86
 *             else
 *               emulator(64)-x86
 *     64-bit: (if '-ranchu')
 *               emulator(64)-ranchu-x86_64 -> qemu/<host>/qemu-system-x86_64
 *             else
 *               emulator(64)-x86
 */
static char*
getTargetEmulatorPath(const char* progDir,
                      bool tryCurrentPath,
                      const char* avdArch,
                      const int force_32bit,
                      bool* is_64bit)
{
    char*  result;
    char* ranchu_result;
    bool search_for_64bit_emulator =
            !force_32bit && android_getHostBitness() == 64;
    const char* emulatorSuffix;

    /* Try look for classic emulator first */
    emulatorSuffix = emulator_getBackendSuffix(avdArch);
    if (!emulatorSuffix) {
        APANIC("This emulator cannot emulate %s CPUs!\n", avdArch);
    }
    D("Looking for emulator-%s to emulate '%s' CPU\n", emulatorSuffix,
      avdArch);

    result = probeTargetEmulatorPath(progDir,
                                     NULL,
                                     emulatorSuffix,
                                     search_for_64bit_emulator,
                                     tryCurrentPath,
                                     is_64bit);
    if (result && !ranchu) {
        /* found and not ranchu */
        D("return result: %s\n", result);
        return result;
    } else {
        /* no classic emulator or prefer ranchu */
        D("Looking for ranchu emulator backend for %s CPU\n", avdArch);
        ranchu_result = probeTargetEmulatorPath(progDir,
                                                "ranchu",
                                                avdArch,
                                                search_for_64bit_emulator,
                                                tryCurrentPath,
                                                is_64bit);
        if (ranchu_result) {
            D("return ranchu: %s\n", ranchu_result);
            return ranchu_result;
        } else {
            if (result) {
                /* ranchu not found, fallback to classic
                   should NOT happen in current scenario
                   just go ahead still and leave a message
                */
                fprintf(stderr, "ERROR: requested 'ranchu' not available\n"
                        "classic backend is used\n");
                return result;
            }
        }
    }

    /* Otherwise, the program is missing */
    APANIC("Missing emulator engine program for '%s' CPUS.\n", avdArch);
    return NULL;
}

static void updateLibrarySearchPath(bool is_64bit) {
    const char* libSubDir = is_64bit ? "lib64" : "lib";
    char fullPath[PATH_MAX];
    char* tail = fullPath;

    char* launcherDir = get_launcher_directory();
    tail = bufprint(fullPath, fullPath + sizeof(fullPath), "%s/%s", launcherDir,
                    libSubDir);
    free(launcherDir);

    if (tail >= fullPath + sizeof(fullPath)) {
        APANIC("Custom library path too long (clipped) [%s]. "
               "Can not use bundled libraries. ",
               fullPath);
    }

    D("Adding library search path: '%s'\n", fullPath);
    add_library_search_dir(fullPath);
}
