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
#include <android/utils/compiler.h>
#include <android/utils/host_bitness.h>
#include <android/utils/panic.h>
#include <android/utils/path.h>
#include <android/utils/bufprint.h>
#include <android/utils/win32_cmdline_quote.h>
#include <android/opengl/emugl_config.h>
#include <android/avd/util.h>

/* Required by android/utils/debug.h */
int android_verbose;


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
static char* getTargetEmulatorPath(const char* progName,
                                   const char* avdArch,
                                   const int force_32bit,
                                   bool* is_64bit);

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

/* Main routine */
int main(int argc, char** argv)
{
    const char* avdName = NULL;
    char*       avdArch = NULL;
    const char* gpu = NULL;
    char*       emulatorPath;
    int         force_32bit = 0;

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

        if (!strcmp(opt,"-force-32bit")) {
            force_32bit = 1;
            continue;
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

    /* Find the architecture-specific program in the same directory */
    bool is_64bit = false;
    emulatorPath = getTargetEmulatorPath(argv[0],
                                         avdArch,
                                         force_32bit,
                                         &is_64bit);
    D("Found target-specific emulator binary: %s\n", emulatorPath);

    /* Replace it in our command-line */
    argv[0] = emulatorPath;

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
    if (!emuglConfig_init(&config, gpuEnabled, gpuMode, gpu, bitness)) {
        fprintf(stderr, "ERROR: %s\n", config.status);
        exit(1);
    }
    D("%s\n", config.status);

    emuglConfig_setupEnv(&config);

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

static char*
getTargetEmulatorPath(const char* progName,
                      const char* avdArch,
                      const int force_32bit,
                      bool* is_64bit)
{
    char*  progDir;
    char*  result;
#ifdef _WIN32
    /* TODO: currently amd64-mingw32msvc-gcc doesn't work which prevents
             generating 64-bit binaries for Windows */
    bool search_for_64bit_emulator = false;
#else
    bool search_for_64bit_emulator =
            !force_32bit && android_getHostBitness() == 64;
#endif

    /* Only search in current path if there is no directory separator
     * in |progName|. */
#ifdef _WIN32
    bool try_current_path =
            (!strchr(progName, '/') && !strchr(progName, '\\'));
#else
    bool try_current_path = !strchr(progName, '/');
#endif

    /* Get program's directory name in progDir */
    path_split(progName, &progDir, NULL);

    const char* emulatorSuffix;

    // Special case: try to find emulator-ranchu-<arch> before emulator-<arch>
    D("Looking for ranchu emulator backed for %s CPU\n", avdArch);
    result = probeTargetEmulatorPath(progDir,
                                     "ranchu",
                                     avdArch,
#ifdef _WIN32
                                     true, // 32 bit ranchu does not work on windows
#else
                                     search_for_64bit_emulator,
#endif
                                     try_current_path,
                                     is_64bit);
    if (result) {
        return result;
    }

    // Special case: for x86_64, first try to find emulator-x86_64 before
    // looking for emulator-x86.
    if (!strcmp(avdArch, "x86_64")) {
        emulatorSuffix = "x86_64";

        D("Looking for emulator backend for %s CPU\n", avdArch);

        result = probeTargetEmulatorPath(progDir,
                                         NULL,
                                         emulatorSuffix,
                                         search_for_64bit_emulator,
                                         try_current_path,
                                         is_64bit);
        if (result) {
            return result;
        }
    }

    // Special case: for arm64, first try to find emulator-arm64 before
    // looking for emulator-arm.
    if (!strcmp(avdArch, "arm64")) {
        emulatorSuffix = "arm64";

        D("Looking for emulator backend for %s CPU\n", avdArch);

        result = probeTargetEmulatorPath(progDir,
                                         NULL,
                                         emulatorSuffix,
                                         search_for_64bit_emulator,
                                         try_current_path,
                                         is_64bit);
        if (result) {
            return result;
        }
    }

    // Now for the regular case.
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
                                     try_current_path,
                                     is_64bit);
    if (result) {
        return result;
    }

    /* Otherwise, the program is missing */
    APANIC("Missing emulator engine program for '%s' CPUS.\n", avdArch);
    return NULL;
}
