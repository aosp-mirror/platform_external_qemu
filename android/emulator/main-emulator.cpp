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
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <unistd.h>
#endif

#include <fstream>
#include <streambuf>

#include "android/avd/scanner.h"
#include "android/avd/util.h"
#include "android/base/ArraySize.h"
#include "android/base/StringView.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/ProcessControl.h"
#include "android/base/system/System.h"
#include "android/camera/camera-list.h"
#include "android/emulation/ConfigDirs.h"
#include "android/main-emugl.h"
#include "android/main-help.h"
#include "android/opengl/emugl_config.h"
#include "android/qt/qt_setup.h"
#include "android/utils/compiler.h"
#include "android/utils/debug.h"
#include "android/utils/exec.h"
#include "android/utils/host_bitness.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"
#include "android/utils/bufprint.h"
#include "android/utils/win32_cmdline_quote.h"
#include "android/version.h"

#include "android/skin/winsys.h"

#ifdef _WIN32
#include <windows.h>
#endif

using android::base::PathUtils;
using android::base::RunOptions;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::ConfigDirs;

#define DEBUG 1

#if DEBUG
#  define D(format, ...)  do { if (android_verbose) printf("emulator: " format "\n", ##__VA_ARGS__); } while (0)
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

/* Forward declarations */
static char* getQemuExecutablePath(const char* programPath,
                                   const char* avdArch,
                                   bool force64bitTarget,
                                   int wantedBitness);

static void updateLibrarySearchPath(int wantedBitness, bool useSystemLibs, const char* launcherDir);

static bool is32bitImageOn64bitRanchuKernel(const char* avdName,
                                             const char* avdArch,
                                             const char* sysDir,
                                             const char* androidOut);

static std::string getAvdSystemPath(const char* avdName, const char* optionalSysPath);

#ifdef _WIN32
static const char kExeExtension[] = ".exe";
#else
static const char kExeExtension[] = "";
#endif

// Return true if string |str| is in |list|, which is an array of string
// pointers of |listSize| elements.
static bool isStringInList(const char* str,
                           const char* const* list,
                           size_t listSize) {
    size_t n = 0;
    for (n = 0; n < listSize; ++n) {
        if (!strcmp(str, list[n])) {
            return true;
        }
    }
    return false;
}

// Return true if the CPU architecture |avdArch| is supported by QEMU2,
// i.e. the 'ranchu' virtual board.
static bool isCpuArchSupportedByRanchu(const char* avdArch) {
    static const char* const kSupported[] =
            {"arm", "arm64", "mips", "mips64", "x86", "x86_64"};
    return isStringInList(avdArch, kSupported, ARRAY_SIZE(kSupported));
}

static std::string emulator_dirname(const std::string& launcherDir) {
    char* cstr1 = path_parent(launcherDir.c_str(), 1);
    char* cstr2 = path_join(cstr1, "emulator");
    std::string cppstr(cstr2);
    free(cstr1);
    free(cstr2);
    return cppstr;
}

static void delete_files(const StringView file_dir, const StringView files_to_delete[],
        unsigned int num_files) {
    for (unsigned int i = 0; i < num_files; ++i) {
        std::string file = PathUtils::join(file_dir, files_to_delete[i]);
        APosixStatus ret = path_delete_file(file.c_str());
        if (ret == 0) {
            D("Deleting file %s done", file.c_str());
        } else {
            D("Deleting file %s failed", file.c_str());
        }
    }
}

static void clean_up_avd_contents_except_config_ini(const char* avd_folder) {
    // sdcard.img should not be deleted, because it is created by sdk manager
    // and we dont know how to re-create it from emulator yet
    // TODO: fixit
    static constexpr StringView files_to_delete[] = {
        "system.img.qcow2", "vendor.img.qcow2", "userdata-qemu.img",
        "userdata-qemu.img.qcow2", "userdata.img", "userdata.img.qcow2", "cache.img",
        "cache.img.qcow2", "version_num.cache", "sdcard.img.qcow2", "encryptionkey.img",
        "encryptionkey.img.qcow2", "hardware-qemu.ini", "emulator-user.ini",
        "default.dtb"
    };
    delete_files(avd_folder, files_to_delete, ARRAY_SIZE(files_to_delete));
}

static void clean_up_android_out(const char* android_out) {
    // note: we should not delete 'userdata.img' otherwise, we will have to run
    // make again to create it; in avd/ folder, it can be copied from
    // system-images/.../<arch>/ directory.
    static constexpr StringView files_to_delete[] = {
        "system.img.qcow2", "vendor.img.qcow2", "userdata-qemu.img",
        "userdata-qemu.img.qcow2", "userdata.img.qcow2", "cache.img.qcow2", "version_num.cache",
        "hardware-qemu.ini", "emulator-user.ini"};
    delete_files(android_out, files_to_delete, ARRAY_SIZE(files_to_delete));
}

static void delete_snapshots_at(const char* content) {
    if (char* const snapshotDir = path_join(content, "snapshots")) {
        if (!path_delete_dir(snapshotDir)) {
            D("Removed snapshot directory '%s'", snapshotDir);
        } else {
            D("Failed to remove snapshot directory '%s'", snapshotDir);
        }
        free(snapshotDir);
    }
}

static void delete_adbCmds_at(const char* content) {
    if (char* const cmdFolder =
            path_join(
                content,
                ANDROID_AVD_TMP_ADB_COMMAND_DIR)) {
        if (!path_delete_dir(cmdFolder)) {
            D("Removed ADB command directory '%s'", cmdFolder);
        } else {
            D("Failed to remove ADB command directory '%s'", cmdFolder);
        }
        free(cmdFolder);
    }
}

static bool checkOsVersion() {
#ifndef _WIN32
    return true;
#else  // _WIN32
    // Make sure OS is Win7+ - otherwise the emulation engine just won't start.
    OSVERSIONINFOW ver = {sizeof(ver)};
    if (!::GetVersionExW(&ver)) {
        const auto err = (unsigned)::GetLastError();
        dwarning(
           "failed to get operating system version.\n"
           "The Android Emulator may not run properly on Windows Vista and\n"
           "won't run on Windows XP (error code %d [0x%x]).",
           err, err);
    } else {
        // Windows 7 is 6.1
        if (ver.dwMajorVersion < 6 ||
            (ver.dwMajorVersion == 6 && ver.dwMinorVersion < 1)) {
            derror(
                "Windows 7 or newer is required to run the Android Emulator.\n"
                "Please upgrade your operating system.");
            return false;
        }
    }
    return true;
#endif  // _WIN32
}

static void doLauncherTest(const char* launcherTestArg);

/* Main routine */
int main(int argc, char** argv)
{
    const char* avdName = NULL;
    const char* avdArch = NULL;
    const char* engine = NULL;
    const char* sysDir = NULL;
    bool doAccelCheck = false;
    bool doListAvds = false;
    bool force32bit = false;
    bool useSystemLibs = false;
    bool forceEngineLaunch = false;
    bool queryVersion = false;
    bool doListWebcams = false;
    bool cleanUpAvdContent = false;
    bool isRestart = false;
    int restartPid = -1;
    bool doDeleteTempDir = false;

    /* Test-only actions */
    bool isLauncherTest = false;
    const char* launcherTestArg = nullptr;

    /* Define ANDROID_EMULATOR_DEBUG to 1 in your environment if you want to
     * see the debug messages from this launcher program.
     */
    const char* debug = getenv("ANDROID_EMULATOR_DEBUG");

    if (debug != NULL && *debug && *debug != '0') {
        base_enable_verbose_logs();
    }

    if (!checkOsVersion()) {
        return 1;
    }

#ifdef __linux__
    /* Define ANDROID_EMULATOR_USE_SYSTEM_LIBS to 1 in your environment if you
     * want the effect of -use-system-libs to be permanent.
     */
    const char* system_libs = getenv("ANDROID_EMULATOR_USE_SYSTEM_LIBS");
    if (system_libs && system_libs[0] && system_libs[0] != '0') {
        useSystemLibs = true;
    }
#endif  // __linux__

    /* Parse command-line and look for
     * 1) an avd name either in the form or '-avd <name>' or '@<name>'
     * 2) '-force-32bit' which always use 32-bit emulator on 64-bit platforms
     * 3) '-verbose'/'-debug-all'/'-debug all'/'-debug-init'/'-debug init'
     *    to enable verbose mode.
     */
    for (int nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];

        if (!strcmp(opt, "-accel-check")) {
            doAccelCheck = true;
            continue;
        }

        if (!strcmp(opt,"-qemu")) {
            forceEngineLaunch = true;
            break;
        }

        if (!strcmp(opt,"-fuchsia")) {
            forceEngineLaunch = true;
            break;
        }

        // NOTE: Process -help options immediately, ignoring all other
        // parameters.
        int helpStatus = emulator_parseHelpOption(opt);
        if (helpStatus >= 0) {
            return helpStatus;
        }

        if (!strcmp(opt, "-version")) {
            queryVersion = true;
            continue;
        }

        if (!strcmp(opt, "-wipe-data")) {
            cleanUpAvdContent = true;
            continue;
        }

        if (!strcmp(opt, "-read-only")) {
            android::base::disableRestart();
            continue;
        }

        if (!strcmp(opt, "-is-restart")) {
            isRestart = true;
            if (nn + 1 < argc) {
                restartPid = atoi(argv[nn + 1]);
                nn++;
            }
            continue;
        }

        if (!strcmp(opt,"-verbose") || !strcmp(opt,"-debug-all")
                 || !strcmp(opt,"-debug-init")) {
            base_enable_verbose_logs();
            continue;
        }

        if (!strcmp(opt,"-debug") && nn + 1 < argc &&
            (!strcmp(argv[nn + 1], "all") || !strcmp(argv[nn + 1], "init"))) {
            base_enable_verbose_logs();
        }

        if (!strcmp(opt,"-gpu") && nn + 1 < argc) {
            nn++;
            continue;
        }

        if (!strcmp(opt,"-ranchu")) {
            // Nothing: the option is deprecated and defaults to auto-detect.
            continue;
        }

        if (!strcmp(opt, "-engine") && nn + 1 < argc) {
            engine = argv[nn + 1];
            nn++;
            continue;
        }

        if (!strcmp(opt,"-force-32bit")) {
            force32bit = true;
            continue;
        }

        if (!strcmp(opt,"-no-window")) {
            continue;
        }

#ifdef __linux__
        if (!strcmp(opt, "-use-system-libs")) {
            useSystemLibs = true;
            continue;
        }
#endif  // __linux__

        if (!strcmp(opt, "-list-avds")) {
            doListAvds = true;
            continue;
        }

        if (!strcmp(opt, "-launcher-test")) {
            isLauncherTest = true;
            launcherTestArg = nullptr;
            if (nn + 1 < argc) {
                launcherTestArg = argv[nn + 1];
            }
            nn++;
            continue;
        }
        if (!strcmp(opt, "--restart")) {
            isRestart = true;
            continue;
        }

        if (!strcmp(opt, "-webcam-list")) {
            doListWebcams = true;
            continue;
        }

        if (!strcmp(opt, "-sysdir") && nn+1 < argc) {
            sysDir = argv[nn+1];
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

        if (!strcmp(opt, "-delete-temp-dir")) {
            doDeleteTempDir = true;
        }
    }

    if (doAccelCheck) {
        // forward the option to our answering machine
        auto& sys = *System::get();
        const auto path = sys.findBundledExecutable("emulator-check");
        if (path.empty()) {
            derror("can't find the emulator-check executable "
                    "(corrupted tools installation?)");
            return -1;
        }

        System::ProcessExitCode exit_code;
        bool ret = sys.runCommand(
                {path, "accel"},
                RunOptions::WaitForCompletion | RunOptions::ShowOutput,
                System::kInfinite, &exit_code);
        return ret ? exit_code : -1;
    }

    if (doListAvds) {
        AvdScanner* scanner = avdScanner_new(NULL);
        for (;;) {
            const char* name = avdScanner_next(scanner);
            if (!name) {
                break;
            }
            printf("%s\n", name);
        }
        avdScanner_free(scanner);
        return 0;
    }

    if (isLauncherTest) {
        if (!launcherTestArg) {
            fprintf(stderr, "ERROR: Launcher test not specified\n");
            return 1;
        }
        doLauncherTest(launcherTestArg);
        fprintf(stderr, "Launcher test complete.\n");
        return 0;
    }

    if (doListWebcams) {
        android_camera_list_webcams();
        return 0;
    }

    if (doDeleteTempDir) {
        System::deleteTempDir();
        return 0;
    }

    /* If ANDROID_EMULATOR_FORCE_32BIT is set to 'true' or '1' in the
     * environment, set -force-32bit automatically.
     */
    {
        const char kEnvVar[] = "ANDROID_EMULATOR_FORCE_32BIT";
        const char* val = getenv(kEnvVar);
        if (val && (!strcmp(val, "true") || !strcmp(val, "1"))) {
            if (!force32bit) {
                D("Auto-config: -force-32bit (%s=%s)", kEnvVar, val);
                force32bit = true;
            }
        }
    }

    int hostBitness = android_getHostBitness();
    int wantedBitness = hostBitness;

#if defined(__linux__)
    // Linux binaries are compiled for 64 bit, so none of the 32 bit settings
    // will make any sense.
    static_assert(sizeof(void*) == 8, "We only support 64 bit in linux (see: c5a2d4198cb)");
    force32bit = false;
    hostBitness = 64;
    wantedBitness = 64;
#endif  // __linux__

    if (force32bit) {
        wantedBitness = 32;
    }

#if defined(__APPLE__)
    // Not sure when the android_getHostBitness will break again
    // but we are not shiping 32bit for OSX long time ago.
    // https://code.google.com/p/android/issues/detail?id=196779
    if (force32bit) {
        fprintf(stderr,
"WARNING: 32-bit OSX Android emulator binaries are not supported, use 64bit.\n");
    }
    wantedBitness = 64;
#endif

    // When running in a platform build environment, point to the output
    // directory where image partition files are located.
    const char* androidOut = NULL;

    // print a version string and build id for easier debugging
#if defined ANDROID_SDK_TOOLS_BUILD_NUMBER
    D("Android emulator version %s (CL:%s)", EMULATOR_VERSION_STRING
      " (build_id " STRINGIFY(ANDROID_SDK_TOOLS_BUILD_NUMBER) ")",
      EMULATOR_CL_SHA1);
#endif

    // If this is a restart, wait for the restartPid to exit.
    if (isRestart && restartPid > -1) {
        System::get()->waitForProcessExit(restartPid, 10000 /* maximum of 10 seconds */);
    }

    /* If there is an AVD name, we're going to extract its target architecture
     * by looking at its config.ini
     */
    if (avdName != NULL) {
        D("Found AVD name '%s'", avdName);
        ScopedCPtr<const char> rootIni(path_getRootIniPath(avdName));
        if (!rootIni) {
            D("path_getRootIniPath(%s) returned NULL", avdName);
            static const char kHomeSearchDir[] =
                    "$HOME" PATH_SEP ".android" PATH_SEP "avd";
            static const char kSdkHomeSearchDir[] =
                    "$ANDROID_SDK_HOME" PATH_SEP "avd";
            const char* envName = "HOME";
            const char* searchDir = kHomeSearchDir;
            if (getenv("ANDROID_AVD_HOME")) {
                envName = "ANDROID_AVD_HOME";
                searchDir = "$ANDROID_AVD_HOME";
            } else if (getenv("ANDROID_SDK_HOME")) {
                envName = "ANDROID_SDK_HOME";
                searchDir = kSdkHomeSearchDir;
            }
            derror("Unknown AVD name [%s], use -list-avds to see valid list.\n"
                   "%s is defined but there is no file %s.ini in %s\n"
                   "(Note: Directories are searched in the order "
                   "$ANDROID_AVD_HOME, %s and %s)",
                   avdName, envName, avdName, searchDir, kSdkHomeSearchDir,
                   kHomeSearchDir);
            return 1;
        }
        avdArch = path_getAvdTargetArch(avdName);
        D("Found AVD target architecture: %s", avdArch);
    } else {
        android::base::disableRestart();

        /* Otherwise, using the ANDROID_PRODUCT_OUT directory */
        androidOut = getenv("ANDROID_PRODUCT_OUT");

        if (androidOut != NULL) {
            D("Found ANDROID_PRODUCT_OUT: %s", androidOut);
            avdArch = path_getBuildTargetArch(androidOut);
            D("Found build target architecture: %s",
              avdArch ? avdArch : "<NULL>");
        }
    }

    if (!avdName && !androidOut && !forceEngineLaunch && !queryVersion) {
        derror("No AVD specified. Use '@foo' or '-avd foo' to launch a virtual"
               " device named 'foo'\n");
        return 1;
    }

    if (cleanUpAvdContent) {
        if (avdName) {
            char* avd_folder = path_getAvdContentPath(avdName);
            if (avd_folder) {
                clean_up_avd_contents_except_config_ini(avd_folder);
                delete_snapshots_at(avd_folder);
                delete_adbCmds_at(avd_folder);
                free(avd_folder);
            }
        } else if (androidOut) {
            clean_up_android_out(androidOut);
            delete_snapshots_at(androidOut);
        }
    }

    if (avdArch == NULL) {
        avdArch = "x86_64";
        D("Can't determine target AVD architecture: defaulting to %s", avdArch);
    }

    /* Find program directory. */
    const auto progDirSystem = android::base::System::get()->getProgramDirectory();
    D("argv[0]: '%s'; program directory: '%s'", argv[0], progDirSystem.c_str());

    if (engine) {
        fprintf(stderr, "WARNING: engine selection is no longer supported, always using RACNHU.\n");
    }

    // Sanity checks.
    if (avdName) {
        if (!isCpuArchSupportedByRanchu(avdArch)) {
            APANIC("CPU Architecture '%s' is not supported by the QEMU2 emulator, (the classic engine is deprecated!)",
                   avdArch);
        }
        std::string systemPath = getAvdSystemPath(avdName, sysDir);
        if (systemPath.empty()) {
            const char* env = getenv("ANDROID_SDK_ROOT");
            if (!env || !env[0]) {
                APANIC("Cannot find AVD system path. Please define "
                       "ANDROID_SDK_ROOT\n");
            } else {
                APANIC("Broken AVD system path. Check your ANDROID_SDK_ROOT "
                       "value [%s]!\n",
                       env);
            }
        }
    }
    // in some cases, progDirSystem is incorrect, so we have to
    // search from the folder from the command line
    // more info can be found at b/65257562
    char* c_argv0_dir_name = path_dirname(argv[0]);
    std::string argv0DirName(c_argv0_dir_name);
    free(c_argv0_dir_name);

    std::string emuDirName = emulator_dirname(progDirSystem);

    D("emuDirName: '%s'", emuDirName.c_str());

    bool force64bitTarget = is32bitImageOn64bitRanchuKernel(avdName, avdArch,
                                                            sysDir, androidOut);
    const StringView candidates[] = {progDirSystem, emuDirName, argv0DirName};
    char* emulatorPath = nullptr;
    StringView progDir;
    for (unsigned int i = 0; i < ARRAY_SIZE(candidates); ++i) {
        D("try dir %s", candidates[i].data());
        progDir = candidates[i];
        emulatorPath = getQemuExecutablePath(
                progDir.data(), avdArch, force64bitTarget, wantedBitness);
        D("Trying emulator path '%s'", emulatorPath);
        if (path_exists(emulatorPath)) {
            break;
        }
        emulatorPath = nullptr;
    }

    if (emulatorPath == nullptr) {
        derror("can't find the emulator executable.\n");
        return -1;
    }

    D("Found target-specific %d-bit emulator binary: %s", wantedBitness, emulatorPath);

    if (avdName) {
        /* Save restart parameters before we modify argv */
        android::base::initializeEmulatorRestartParameters(argc, argv, path_getAvdContentPath(avdName));
    }

    /* Replace it in our command-line */
    argv[0] = emulatorPath;

    /* setup launcher dir */
    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR", progDir);

    /* Setup library paths so that bundled standard shared libraries are picked
     * up by the re-exec'ed emulator
     */
    updateLibrarySearchPath(wantedBitness, useSystemLibs, progDir.data());

    /* We need to find the location of the GLES emulation shared libraries
     * and modify either LD_LIBRARY_PATH or PATH accordingly
     */

    /* Add <lib>/qt/ to the library search path. */
    androidQtSetupEnv(wantedBitness, progDir.data());

#ifdef _WIN32
    // Take care of quoting all parameters before sending them to execv().
    // See the "Eveyone quotes command line arguments the wrong way" on
    // MSDN.
    int n;
    for (n = 0; n < argc; ++n) {
        // Technically, this leaks the quoted strings, but we don't care
        // since this process will terminate after the execv() anyway.
        argv[n] = win32_cmdline_quote(argv[n]);
        D("Quoted param: [%s]", argv[n]);
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

// Convert an emulator-specific CPU architecture name |avdArch| into the
// corresponding QEMU one. Return NULL if unknown.
static const char* getQemuArch(const char* avdArch, bool force64bitTarget) {
    static const struct {
        const char* arch;
        const char* qemuArch;
    } kQemuArchs[] = {
        {"arm", "armel"},
        {"arm64", "aarch64"},
        {"arm64", "aarch64"},
        {"mips", "mipsel"},
        {"mips64", "mips64el"},
        {"mips64", "mips64el"},
        {"x86","i386"},
        {"x86_64","x86_64"},
        {"x86_64","x86_64"},
    };
    size_t n;
    for (n = 0; n < ARRAY_SIZE(kQemuArchs); ++n) {
        if (!strcmp(avdArch, kQemuArchs[n].arch)) {
            if (force64bitTarget) {
                return kQemuArchs[n+1].qemuArch;
            } else {
                return kQemuArchs[n].qemuArch;
            }
        }
    }
    return NULL;
}

// Return the path of the QEMU executable. |progDir| is the directory
// containing the current program. |avdArch| is the CPU architecture name.
// of the current program (i.e. the 'emulator' launcher).
// Return NULL in case of error.
static char* getQemuExecutablePath(const char* progDir,
                                   const char* avdArch,
                                   bool force64bitTarget,
                                   int wantedBitness) {
// The host operating system name.
#ifdef __linux__
    static const char kHostOs[] = "linux";
#elif defined(__APPLE__)
    static const char kHostOs[] = "darwin";
#elif defined(_WIN32)
    static const char kHostOs[] = "windows";
#endif
    const char* hostArch = (wantedBitness == 64) ? "x86_64" : "x86";
    const char* qemuArch = getQemuArch(avdArch, force64bitTarget);
    if (!qemuArch) {
        APANIC("QEMU2 emulator does not support %s CPU architecture", avdArch);
    }

    char fullPath[PATH_MAX];
    char* fullPathEnd = fullPath + sizeof(fullPath);
    char* tail = bufprint(fullPath,
                          fullPathEnd,
                          "%s" PATH_SEP "qemu" PATH_SEP "%s-%s" PATH_SEP "qemu-system-%s%s",
                          progDir,
                          kHostOs,
                          hostArch,
                          qemuArch,
                          kExeExtension);
    if (tail >= fullPathEnd) {
        APANIC("QEMU executable path too long (clipped) [%s]. "
               "Can not use QEMU2 emulator. ", fullPath);
    }

    return path_get_absolute(fullPath);
}

static void updateLibrarySearchPath(int wantedBitness, bool useSystemLibs, const char* launcherDir) {
    const char* libSubDir = (wantedBitness == 64) ? "lib64" : "lib";
    char fullPath[PATH_MAX];
    char* tail = fullPath;

    tail = bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s", launcherDir,
                    libSubDir);

    if (tail >= fullPath + sizeof(fullPath)) {
        APANIC("Custom library path too long (clipped) [%s]. "
               "Can not use bundled libraries. ",
               fullPath);
    }

    D("Adding library search path: '%s'", fullPath);
    add_library_search_dir(fullPath);

    bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_swiftshader");
    D("Adding library search path: '%s'", fullPath);
    add_library_search_dir(fullPath);

    bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle");
    D("Adding library search path: '%s'", fullPath);
    add_library_search_dir(fullPath);

    bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle9");
    D("Adding library search path: '%s'", fullPath);
    add_library_search_dir(fullPath);

    bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle11");
    D("Adding library search path: '%s'", fullPath);
    add_library_search_dir(fullPath);

#ifdef __linux__
    if (!useSystemLibs) {
        // Use bundled libstdc++
        tail = bufprint(fullPath, fullPath + sizeof(fullPath),
                        "%s/%s/libstdc++", launcherDir, libSubDir);

        if (tail >= fullPath + sizeof(fullPath)) {
            APANIC("Custom library path too long (clipped) [%s]. "
                "Can not use bundled libraries. ",
                fullPath);
        }

        D("Adding library search path: '%s'", fullPath);
        add_library_search_dir(fullPath);
    }
#else  // !__linux__
    (void)useSystemLibs;
#endif  // !__linux__
}

// Return the system directory path of a given AVD named |avdName|.
// If |optionalSysPath| is non-null, this simply returns that. Otherwise
// this searches for a valid path.
// Return empty string on failure.
static std::string getAvdSystemPath(const char* avdName, const char* optionalSysPath) {
    std::string result;
    if (!avdName) {
        return result;
    }
    if (optionalSysPath) {
        result = optionalSysPath;
        return result;
    }
    char* sdkRootPath = path_getSdkRoot();
    if (!sdkRootPath) {
        return result;
    }
    char* systemPath = path_getAvdSystemPath(avdName, sdkRootPath);
    if (systemPath != nullptr) {
         result = systemPath;
         AFREE(systemPath);
    }
    AFREE(sdkRootPath);
    return result;
}

// check for 32bit image running on 64bit ranchu kernel
static bool is32bitImageOn64bitRanchuKernel(const char* avdName,
                                               const char* avdArch,
                                               const char* sysDir,
                                               const char* androidOut) {
    // only appliable to 32bit image
    if (strcmp(avdArch, "x86") && strcmp(avdArch, "arm") && strcmp(avdArch, "mips")) {
        return false;
    }

    bool result = false;
    char* kernel_file = NULL;
    if (androidOut) {
        asprintf(&kernel_file, "%s" PATH_SEP "kernel-ranchu-64", androidOut);
    } else {
        std::string systemImagePath = getAvdSystemPath(avdName, sysDir);
        asprintf(&kernel_file, "%s" PATH_SEP "%s", systemImagePath.c_str(),
                 "kernel-ranchu-64");
    }
    result = path_exists(kernel_file);
    D("Probing for %s: file %s", kernel_file, result ? "exists" : "missing");

    AFREE(kernel_file);
    return result;
}

static constexpr char existsStr[] = "(exists)";
static constexpr char notexistStr[] = "(does not exist)";

static const char* getExistsStr(bool exists) {
    return exists ? existsStr : notexistStr;
}

static void doLauncherTest(const char* launcherTestArg) {
    if (!launcherTestArg || !launcherTestArg[0]) {
        printf("Error: No launcher test specified.\n");
    }

    if (!strcmp(launcherTestArg, "sdkCheck")) {
        auto sdkRoot = ConfigDirs::getSdkRootDirectory();
        auto avdRoot = ConfigDirs::getAvdRootDirectory();

        bool sdkRootExists = path_exists(sdkRoot.c_str());
        bool avdRootExists = path_exists(avdRoot.c_str());

        printf("Performing SDK check.\n");
        printf("Android SDK location: %s. %s\n",
               sdkRoot.c_str(), getExistsStr(sdkRootExists));
        printf("Android AVD root location: %s. %s\n",
               avdRoot.c_str(), getExistsStr(avdRootExists));
        return;
    }

    // TODO: Other launcher tests

    printf("Error: Unknown launcher test %s\n", launcherTestArg);
}
