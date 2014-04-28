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
#include <android/utils/panic.h>
#include <android/utils/path.h>
#include <android/utils/bufprint.h>
#include <android/utils/win32_cmdline_quote.h>
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

#if defined(__x86_64__)
/* Normally emulator is compiled in 32-bit.  In standalone it can be compiled
   in 64-bit (with ,/android-configure.sh --try-64).  In this case, emulator-$ARCH
   are also compiled in 64-bit and will search for lib64*.so instead of lib*so */
#define  GLES_EMULATION_LIB  "lib64OpenglRender" DLL_EXTENSION
#elif defined(__i386__)
#define  GLES_EMULATION_LIB  "libOpenglRender" DLL_EXTENSION
#else
#error Unknown architecture for codegen
#endif


/* Forward declarations */
static char* getTargetEmulatorPath(const char* progName, const char* avdArch, const int force_32bit);
static char* getSharedLibraryPath(const char* progName, const char* libName);
static void  prependSharedLibraryPath(const char* prefix);

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
    emulatorPath = getTargetEmulatorPath(argv[0], avdArch, force_32bit);
    D("Found target-specific emulator binary: %s\n", emulatorPath);

    /* Replace it in our command-line */
    argv[0] = emulatorPath;

    /* We need to find the location of the GLES emulation shared libraries
     * and modify either LD_LIBRARY_PATH or PATH accordingly
     */
    {
        char*  sharedLibPath = getSharedLibraryPath(emulatorPath, GLES_EMULATION_LIB);

        if (sharedLibPath != NULL) {
            D("Found OpenGLES emulation libraries in %s\n", sharedLibPath);
            prependSharedLibraryPath(sharedLibPath);
        } else {
            D("Could not find OpenGLES emulation host libraries!\n");
        }
    }

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

#ifndef _WIN32
static int
getHostOSBitness()
{
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
}
#endif  // !_WIN32

/* Find the target-specific emulator binary. This will be something
 * like  <programDir>/emulator-<targetArch>, where <programDir> is
 * the directory of the current program.
 */
static char*
getTargetEmulatorPath(const char* progName, const char* avdArch, const int force_32bit)
{
    char*  progDir;
    char   path[PATH_MAX], *pathEnd=path+sizeof(path), *p;
    const char* emulatorPrefix = "emulator-";
    const char* emulator64Prefix = "emulator64-";
#ifdef _WIN32
    const char* exeExt = ".exe";
    /* ToDo: currently amd64-mingw32msvc-gcc doesn't work (http://b/issue?id=5949152)
             which prevents us from generating 64-bit emulator for Windows */
    int search_for_64bit_emulator = 0;
#else
    const char* exeExt = "";
    int search_for_64bit_emulator = !force_32bit && getHostOSBitness() == 64;
#endif

    const char* emulatorSuffix = emulator_getBackendSuffix(avdArch);
    if (!emulatorSuffix) {
        APANIC("This emulator cannot emulate %s CPUs!\n", avdArch);
    }
    D("Using emulator-%s to emulate '%s' CPUs\n", emulatorSuffix, avdArch);

    /* Get program's directory name in progDir */
    path_split(progName, &progDir, NULL);

    if (search_for_64bit_emulator) {
        /* Find 64-bit emulator first */
        p = bufprint(path, pathEnd, "%s/%s%s%s", progDir,
                     emulator64Prefix, emulatorSuffix, exeExt);
        if (p >= pathEnd) {
            APANIC("Path too long: %s\n", progName);
        }
        if (path_exists(path)) {
            free(progDir);
            return strdup(path);
        }
    }

    /* Find 32-bit emulator */
    p = bufprint(path, pathEnd, "%s/%s%s%s", progDir, emulatorPrefix,
                 emulatorSuffix, exeExt);
    free(progDir);
    if (p >= pathEnd) {
        APANIC("Path too long: %s\n", progName);
    }

    if (path_exists(path)) {
        return strdup(path);
    }

    /* Mmm, the file doesn't exist, If there is no slash / backslash
     * in our path, we're going to try to search it in our path.
     */
#ifdef _WIN32
    if (strchr(progName, '/') == NULL && strchr(progName, '\\') == NULL) {
#else
    if (strchr(progName, '/') == NULL) {
#endif
        if (search_for_64bit_emulator) {
           p = bufprint(path, pathEnd, "%s%s%s", emulator64Prefix,
                        emulatorSuffix, exeExt);
           if (p < pathEnd) {
               char*  resolved = path_search_exec(path);
               if (resolved != NULL)
                   return resolved;
           }
        }

        p = bufprint(path, pathEnd, "%s%s%s", emulatorPrefix,
                     emulatorSuffix, exeExt);
        if (p < pathEnd) {
            char*  resolved = path_search_exec(path);
            if (resolved != NULL)
                return resolved;
        }
    }

    /* Otherwise, the program is missing */
    APANIC("Missing arch-specific emulator program: %s\n", path);
    return NULL;
}

/* return 1 iff <path>/<filename> exists */
static int
probePathForFile(const char* path, const char* filename)
{
    char  temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    p = bufprint(temp, end, "%s/%s", path, filename);
    D("Probing for: %s\n", temp);
    return (p < end && path_exists(temp));
}

/* Find the directory containing a given shared library required by the
 * emulator (for GLES emulation). We will probe several directories
 * that correspond to various use-cases.
 *
 * Caller must free() result string. NULL if not found.
 */

static char*
getSharedLibraryPath(const char* progName, const char* libName)
{
    char* progDir;
    char* result = NULL;
    char  temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    /* Get program's directory name */
    path_split(progName, &progDir, NULL);

    /* First, try to probe the program's directory itself, this corresponds
     * to the standalone build with ./android-configure.sh where the script
     * will copy the host shared library under external/qemu/objs where
     * the binaries are located.
     */
    if (probePathForFile(progDir, libName)) {
        return progDir;
    }

    /* Try under $progDir/lib/, this should correspond to the SDK installation
     * where the binary is under tools/, and the libraries under tools/lib/
     */
    {
        p = bufprint(temp, end, "%s/lib", progDir);
        if (p < end && probePathForFile(temp, libName)) {
            result = strdup(temp);
            goto EXIT;
        }
    }

    /* try in $progDir/../lib, this corresponds to the platform build
     * where the emulator binary is under out/host/<system>/bin and
     * the libraries are under out/host/<system>/lib
     */
    {
        char* parentDir = path_parent(progDir, 1);

        if (parentDir == NULL) {
            parentDir = strdup(".");
        }
        p = bufprint(temp, end, "%s/lib", parentDir);
        free(parentDir);
        if (p < end && probePathForFile(temp, libName)) {
            result = strdup(temp);
            goto EXIT;
        }
    }

    /* Nothing found! */
EXIT:
    free(progDir);
    return result;
}

/* Prepend the path in 'prefix' to either LD_LIBRARY_PATH or PATH to
 * ensure that the shared libraries inside the path will be available
 * through dlopen() to the emulator program being launched.
 */
static void
prependSharedLibraryPath(const char* prefix)
{
    size_t len = 0;
    char *temp = NULL;
    const char* path = NULL;

#ifdef _WIN32
    path = getenv("PATH");
#else
    path = getenv("LD_LIBRARY_PATH");
#endif

    /* Will need up to 7 extra characters: "PATH=", ';' or ':', and '\0' */
    len = 7 + strlen(prefix) + (path ? strlen(path) : 0);
    temp = malloc(len);
    if (!temp)
        return;

    if (path && path[0] != '\0') {
#ifdef _WIN32
        bufprint(temp, temp + len, "PATH=%s;%s", prefix, path);
#else
        bufprint(temp, temp + len, "%s:%s", prefix, path);
#endif
    } else {
#ifdef _WIN32
        bufprint(temp, temp + len, "PATH=%s", prefix);
#else
        strcpy(temp, prefix);
#endif
    }

#ifdef _WIN32
    D("Setting %s\n", temp);
    putenv(strdup(temp));
#else
    D("Setting LD_LIBRARY_PATH=%s\n", temp);
    setenv("LD_LIBRARY_PATH", temp, 1);
#endif
}
