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

#include "android/HostHwInfo.h"
#include "android/avd/info.h"
#include "android/avd/scanner.h"
#include "android/avd/util.h"
#include "android/base/ArraySize.h"
#include "android/base/ProcessControl.h"
#include "android/base/StringView.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/camera/camera-list.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/USBAssist.h"
#include "android/main-emugl.h"
#include "android/main-help.h"
#include "android/opengl/emugl_config.h"
#include "android/qt/qt_setup.h"
#include "android/utils/bufprint.h"
#include "android/utils/compiler.h"
#include "android/utils/debug.h"
#include "android/utils/exec.h"
#include "android/utils/host_bitness.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"
#include "android/utils/win32_cmdline_quote.h"
#include "android/version.h"

#include "android/skin/winsys.h"

#ifdef __linux__
#include <fcntl.h>
#if defined(__aarch64__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#endif
#endif  // __linux__

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

using android::base::PathUtils;
using android::base::RunOptions;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::ConfigDirs;

#define DEBUG 1

#if DEBUG
#  define D(...)  do { if (android_verbose) dinfo(__VA_ARGS__); } while (0)
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
static char* getClassicEmulatorPath(const char* progDir,
                                    const char* avdArch,
                                    int* wantedBitness);

static char* getQemuExecutablePath(const char* programPath,
                                   const char* avdArch,
                                   bool force64bitTarget,
                                   int wantedBitness,
                                   bool isHeadless);

static void updateLibrarySearchPath(bool isHeadless,
                                    int wantedBitness,
                                    bool useSystemLibs,
                                    const char* launcherDir,
                                    const char* gpu);

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

#ifdef __linux__
#if defined(__aarch64__)
static void setupCpuAffinity(
        const android::HostHwInfo::ArmCpuInfo& armCpuInfo) {
    cpu_set_t mycpus;
    CPU_ZERO(&mycpus);
    CPU_SET(0, &mycpus);
    for (size_t i = 1; i < armCpuInfo.cpumodels.size(); ++i) {
        if (armCpuInfo.cpumodels[i] != armCpuInfo.cpumodels[0]) {
            break;
        }
        CPU_SET(i, &mycpus);
    }
    sched_setaffinity(getpid(), sizeof(mycpus), &mycpus);
}
#endif
#endif

static void doLauncherTest(const char* launcherTestArg);

#if defined(__APPLE__)
// Use "sysctl.proc_translated" to check if running in Rosetta
// Returns 1 if running in Rosetta
static int processIsTranslated() {
    int ret = 0;
    size_t size = sizeof(ret);
    // Call the sysctl and if successful return the result
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) != -1) {
        return ret;
    }
    // If "sysctl.proc_translated" is not present then must be native
    if (errno == ENOENT) {
        return 0;
    }

    // Fall back to native
    return 0;
}
#endif

/* Main routine */
int main(int argc, char** argv)
{
    const char* avdName = NULL;
    const char* avdArch = NULL;
    const char* engine = NULL;
    const char* sysDir = NULL;
    const char* gpu = NULL;
    bool doAccelCheck = false;
    bool doListAvds = false;
    bool doListUSB = false;
    bool force32bit = false;
    bool isHeadless = false;
    bool useSystemLibs = false;
    bool forceEngineLaunch = false;
    bool isFuchsia = false;
    bool queryVersion = false;
    bool doListWebcams = false;
    bool cleanUpAvdContent = false;
    bool isRestart = false;
    int restartPid = -1;
    bool doDeleteTempDir = false;
    bool checkLoadable = false;
    bool use_virtio_console = false;

#ifdef __APPLE__
    if (processIsTranslated()) {
        fprintf(stderr, "emulator: ERROR: process is translated under Rosetta. Attempting to replace emulator installation.\n");
        const auto progDirSystem = android::base::System::get()->getProgramDirectory();
        const auto cmd = PathUtils::join(progDirSystem, "darwin-aarch64-replace.sh");
        fprintf(stderr, "emulator: Replacing via command: %s (downloading ~120 MB)...\n", cmd.c_str());
        system(cmd.c_str());
        fprintf(stderr, "emulator: Replacement done. Please relaunch the emulator. You will also need to be using an Apple Silicon-compatible system image. Check the release updates blog (https://androidstudio.googleblog.com/) for more details.\n");
        return -1;
    }
#endif

    const char* qemu_top_dir = nullptr;
    for (int nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];

#ifdef __linux__
#if defined(__aarch64__)
        if (!strcmp(opt, "-virtio-console")) {
            use_virtio_console = true;
            continue;
        }
#endif
#endif

        if (!strcmp(opt, "-qemu-top-dir") && nn + 1 < argc) {
            qemu_top_dir = argv[nn + 1];
            // shuffle up the arguments
            for (int jj = nn + 2; jj < argc; nn++, jj++) {
                argv[nn] = argv[jj];
            }
            argc -= 2;
            argv[argc] = nullptr;
            break;
        }
        // quick check for qemu1
        if (!strcmp(opt, "-engine") && nn + 1 < argc && !strcmp(argv[nn + 1], "classic")) {
            std::string qemu1path = std::string(path_dirname(argv[0])) + PATH_SEP + "qemu1"
                + PATH_SEP + "emulator";
            if (path_exists(qemu1path.c_str())) {
                qemu_top_dir = "qemu1";
                break;
            }
        }
    }
    if (qemu_top_dir) {
        char mybuf[1024];
        char* c_argv0_dir_name = path_dirname(argv[0]);
        snprintf(mybuf, sizeof(mybuf), "%s" PATH_SEP "%s" PATH_SEP "emulator",
                c_argv0_dir_name, qemu_top_dir);
        char* emulatorPath = mybuf;
        if (!path_exists(emulatorPath)) {
            // try absolute path
            snprintf(mybuf, sizeof(mybuf), "%s" PATH_SEP "emulator", qemu_top_dir);
            emulatorPath = mybuf;
            if (!path_exists(emulatorPath)) {
                fprintf(stderr, "emulator: Cannot find %s\n", emulatorPath);
                return -1;
            }
        }
        argv[0] = emulatorPath;
        printf("emulator: INFO: launch %s ... \n", emulatorPath);
        fflush(stdout);
        safe_execv(emulatorPath, argv);
        return errno;
    }

    /* Test-only actions */
    bool isLauncherTest = false;
    const char* launcherTestArg = nullptr;
    /*
     * Always override LC_ALL = C. Fixes b/123689918
     */
    System::get()->envSet("LC_ALL", "C");

    /* Set MESA_RGB_VISUAL to something that will work on Linux */
    System::get()->envSet("MESA_RGB_VISUAL", "TrueColor 24");

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
    const char* stdouterr_file = nullptr;

#if defined(__aarch64__)
    // check big-little
    const android::HostHwInfo::ArmCpuInfo& armCpuInfo =
            android::HostHwInfo::queryArmCpuInfo();
    if (android::HostHwInfo::queryArmCpuInfo().is_big_little) {
        // set up cpu affinity
        setupCpuAffinity(armCpuInfo);
    }
    if (use_virtio_console) {
        System::get()->envSet("ANDROID_EMULATOR_USE_VIRTIO_CONSOLE", "1");
    }
#endif

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
            isFuchsia = true;
            break;
        }

#ifdef __linux__
        if (!strcmp(opt, "-stdouterr-file")) {
            stdouterr_file = argv[nn + 1];
            nn++;
            continue;
        }
#endif  // __linux__

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

         if (!strcmp(opt,"-debug-log")) {
             VERBOSE_ENABLE(log);
         }

        if (!strcmp(opt,"-gpu") && nn + 1 < argc) {
            gpu = argv[nn + 1];
            nn++;
            continue;
        }

        if (!strcmp(opt,"-allow-host-audio")) {
            continue;
        }

        if (!strcmp(opt,"-restart-when-stalled")) {
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

        if (!strcmp(opt,"-no-qt")) {
            isHeadless = true;
            continue;
        }

        if (!strcmp(opt,"-no-window")) {
            isHeadless = true;
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

#ifdef _WIN32
        if (!strcmp(opt, "-list-usb")) {
            doListUSB = true;
            continue;
        }
#endif // _WIN32

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

        if (!strcmp(opt, "-avd-arch") && nn+1 < argc) {
            avdArch = argv[nn+1];
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

        if (!strcmp(opt, "-check-snapshot-loadable")) {
            checkLoadable = true;
        }
    }
    if (checkLoadable) {
        cleanUpAvdContent = false;
    }

#ifdef __linux__
    if (stdouterr_file) {
        int myfd = open(stdouterr_file,  O_APPEND | O_WRONLY);
        if (myfd < 0) {
            fprintf(stderr, "cannot open %s\n", stdouterr_file);
            return -1;
        }
        if (dup2(myfd, 1) < 0) {
            fprintf(stderr, "cannot dup stdout\n");
            return -1;
        }
        if (dup2(myfd, 2) < 0) {
            fprintf(stderr, "cannot dup stderr\n");
            return -1;
        }
        fprintf(stderr, "emulator: Redirecting stdout/stderr to %s\n", stdouterr_file);
    }
#endif  // __linux__

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

#ifdef _WIN32
    if (doListUSB) {
        listUSBDevices();
        return 0;
    }
#endif //_WIN32

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
    dinfo("Android emulator version %s (CL:%s)", EMULATOR_VERSION_STRING
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
            derror("Unknown AVD name [%s], use -list-avds to see valid list.",avdName);
            derror("%s is defined but there is no file %s.ini in %s", envName, avdName, searchDir);
            derror("(Note: Directories are searched in the order $ANDROID_AVD_HOME, %s and %s)",
                    kSdkHomeSearchDir, kHomeSearchDir);
            return 1;
        }
        avdArch = path_getAvdTargetArch(avdName);
        D("Found AVD target architecture: %s", avdArch);
    } else if (avdArch != NULL) {
        android::base::disableRestart();
        D("Using provided target architecture: %s", avdArch);
    } else {
        android::base::disableRestart();

        if (!isFuchsia) {
            /* Otherwise, using the ANDROID_PRODUCT_OUT directory */
            androidOut = getenv("ANDROID_PRODUCT_OUT");

            if (androidOut != NULL) {
                D("Found ANDROID_PRODUCT_OUT: %s", androidOut);
                avdArch = path_getBuildTargetArch(androidOut);
                D("Found build target architecture: %s",
                  avdArch ? avdArch : "<NULL>");
            }
        }
    }

    if (!avdName && !avdArch && !androidOut && !forceEngineLaunch && !queryVersion) {
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
#ifdef __aarch64__
        avdArch = "arm64";
#else
        avdArch = "x86_64";
#endif
        D("Can't determine target AVD architecture: defaulting to %s", avdArch);
    }

    /* Find program directory. */
    const auto progDirSystem = android::base::System::get()->getProgramDirectory();
    D("argv[0]: '%s'; program directory: '%s'", argv[0], progDirSystem.c_str());

    enum RanchuState {
        RANCHU_ON,
        RANCHU_OFF,
    } ranchu = RANCHU_ON;

    if (engine) {
        if (!strcmp(engine, "classic")) {
            ranchu = RANCHU_OFF;
        }
        fprintf(stderr, "WARNING: engine selection is deprecated.\n");
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

    if (avdName) {
      AvdInfoParams myparams;
      AvdInfo *myavdinfo = avdInfo_new(avdName, &myparams);
      if (avdInfo_getAvdFlavor(myavdinfo) == AVD_ANDROID_AUTO) {
        const char* forge = getenv("TEST_UNDECLARED_OUTPUTS_DIR");
        if (forge != NULL && *forge && *forge != '0') {
          isHeadless = true;
          D("force headless for auto on forge");
        }
      }
      const int apiLevel = avdInfo_getApiLevel(myavdinfo);
      {
          char* avdarch = avdInfo_getTargetCpuArch(myavdinfo);
          const std::string sarch(avdarch);
#ifdef __x86_64__
          if (sarch == "arm64" && apiLevel >=28) {
              APANIC("Avd's CPU Architecture '%s' is not supported by the QEMU2 emulator on x86_64 host.\n", avdarch);
          }
#endif
#if defined(__aarch64__)
          if (sarch != "arm64") {
              APANIC("Avd's CPU Architecture '%s' is not supported by the QEMU2 emulator on aarch64 host.\n", avdarch);
          }
#endif
          free(avdarch);
      }
    }

    bool force64bitTarget = is32bitImageOn64bitRanchuKernel(avdName, avdArch,
                                                            sysDir, androidOut);
    const StringView candidates[] = {progDirSystem, emuDirName, argv0DirName};
    char* emulatorPath = nullptr;
    StringView progDir;
    for (unsigned int i = 0; i < ARRAY_SIZE(candidates); ++i) {
        D("try dir %s", candidates[i].data());
        progDir = candidates[i];
        if (ranchu == RANCHU_ON) {
            emulatorPath = getQemuExecutablePath(
                    progDir.data(), avdArch, force64bitTarget,
                    wantedBitness, isHeadless);
        } else {
            emulatorPath = getClassicEmulatorPath(progDir.data(), avdArch,
                                                  &wantedBitness);
        }
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

    if (avdName || androidOut) {
        /* Save restart parameters before we modify argv */
        android::base::initializeEmulatorRestartParameters(argc, argv,
                avdName ? path_getAvdContentPath(avdName) : androidOut);
    }

    /* Replace it in our command-line */
    argv[0] = emulatorPath;

    /* setup launcher dir */
    System::get()->envSet("ANDROID_EMULATOR_LAUNCHER_DIR", progDir);

    /* Setup library paths so that bundled standard shared libraries are picked
     * up by the re-exec'ed emulator
     */
    updateLibrarySearchPath(isHeadless, wantedBitness, useSystemLibs,
                            progDir.data(), gpu);

    /* We need to find the location of the GLES emulation shared libraries
     * and modify either LD_LIBRARY_PATH or PATH accordingly
     */

    if (!isHeadless) {
        /* Add <lib>/qt/ to the library search path. */
        androidQtSetupEnv(wantedBitness, progDir.data());
    } else {
        /* Disable crash reporter in headless mode, as the crash reporter
         * itself depends on Qt
         * TODO: Have a text-only crash reporter */
        System::get()->envSet("ANDROID_EMU_ENABLE_CRASH_REPORTING", "0");
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
        D("Quoted param: [%s]", argv[n]);
    }
#endif

    if (android_verbose) {
        int i;
        dprint("emulator: Running :%s", emulatorPath);
        for(i = 0; i < argc; i++) {
            dprint("qemu backend: argv[%02d] = \"%s\"", i, argv[i]);
        }
        std::string concat;
        for (i = 0; i < argc; i++) {
            /* To make it easier to copy-paste the output to a command-line,
             * quote anything that contains spaces.
             */
            if (strchr(argv[i], ' ') != NULL) {
                concat += " '" + std::string(argv[i]) + " '";
            } else {
                concat += " " + std::string(argv[i]);
            }
        }
         /* Dump final command-line parameters to make debugging easier */
        dprint("Concatenated backend parameters: %s", concat.c_str());
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
                                   const char* archSuffix) {
    if (progDir) {
        p = bufprint(p, end, "%s" PATH_SEP, progDir);
    }
    p = bufprint(p, end, "%s%s%s", prefix, archSuffix, kExeExtension);
    return p;
}

/* Probe the filesystem to check if an emulator executable named like
 * <progDir>/<prefix><arch> exists.
 *
 * |progDir| is an optional program directory. If NULL, the executable
 * will be searched in the current directory.
 * |archSuffix| is an architecture-specific suffix, like "arm", or 'x86"
 * |wantedBitness| points to an integer describing the wanted bitness of
 * the program. The function might modify it, in the case where it is 64
 * but only 32-bit versions of the executables are found (in this case,
 * |*wantedBitness| is set to 32).
 * On success, returns the absolute path of the executable (string must
 * be freed by the caller). On failure, return NULL.
 */
static char* probeTargetEmulatorPath(const char* progDir,
                                     const char* archSuffix,
                                     int* wantedBitness) {
    char path[PATH_MAX], *pathEnd = path + sizeof(path), *p;

    static const char kEmulatorPrefix[] = "emulator-";
    static const char kEmulator64Prefix[] = "emulator64-";

    // First search for the 64-bit emulator binary.
    if (*wantedBitness == 64) {
        p = bufprint_emulatorName(path,
                                  pathEnd,
                                  progDir,
                                  kEmulator64Prefix,
                                  archSuffix);
        D("Probing program: %s", path);
        if (p < pathEnd && path_exists(path)) {
            return strdup(path);
        }
    }

    // Then for the 32-bit one.
    p = bufprint_emulatorName(path,
                                pathEnd,
                                progDir,
                                kEmulatorPrefix,
                                archSuffix);
    D("Probing program: %s", path);
    if (p < pathEnd && path_exists(path)) {
        *wantedBitness = 32;
        return path_get_absolute(path);
    }

    return NULL;
}

// Find the absolute path to the classic emulator binary that supports CPU architecture
// |avdArch|. |progDir| is the program's directory.
static char* getClassicEmulatorPath(const char* progDir,
                                    const char* avdArch,
                                    int* wantedBitness) {
    const char* emulatorSuffix = emulator_getBackendSuffix(avdArch);
    if (!emulatorSuffix) {
        APANIC("This emulator cannot emulate %s CPUs!\n", avdArch);
    }
    D("Looking for emulator-%s to emulate '%s' CPU", emulatorSuffix,
      avdArch);

    char* result = probeTargetEmulatorPath(progDir,
                                           emulatorSuffix,
                                           wantedBitness);
    if (!result) {
        APANIC("Missing emulator engine program for '%s' CPU.\n", avdArch);
    }
    D("return result: %s", result);
    return result;
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
                                   int wantedBitness,
                                   bool isHeadless) {
// The host operating system name.
#ifdef __linux__
    static const char kHostOs[] = "linux";
#elif defined(__APPLE__)
    static const char kHostOs[] = "darwin";
#elif defined(_WIN32)
    static const char kHostOs[] = "windows";
#endif
#if defined(__aarch64__)
    const char* hostArch = "aarch64";
#else
    const char* hostArch = (wantedBitness == 64) ? "x86_64" : "x86";
#endif
    const char* qemuArch = getQemuArch(avdArch, force64bitTarget);
    if (!qemuArch) {
        APANIC("QEMU2 emulator does not support %s CPU architecture", avdArch);
    }

#define QEMU_BINARY_PATTERN_HEADLESS "qemu-system-%s-headless%s"
#define QEMU_BINARY_PATTERN "qemu-system-%s%s"

    char fullPath[PATH_MAX];
    char* fullPathEnd = fullPath + sizeof(fullPath);
    const char* qemuStandardPath =
        "%s" PATH_SEP "qemu" PATH_SEP "%s-%s" PATH_SEP QEMU_BINARY_PATTERN;
    const char* qemuHeadlessPath =
        "%s" PATH_SEP "qemu" PATH_SEP "%s-%s" PATH_SEP QEMU_BINARY_PATTERN_HEADLESS;

    char* tail = bufprint(fullPath,
                          fullPathEnd,
                          isHeadless ? qemuHeadlessPath : qemuStandardPath,
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

static void appendPreloadLib(const char* fullLibPath) {
    if (!fullLibPath)
        return;

    std::string real_preload(fullLibPath);
    const char* current_preload = getenv("LD_PRELOAD");
    if (current_preload) {
        real_preload = real_preload + " " + std::string(fullLibPath);
    }
    System::get()->envSet("LD_PRELOAD", real_preload.c_str());
}

static void updateLibrarySearchPath(bool isHeadless,
                                    int wantedBitness,
                                    bool useSystemLibs,
                                    const char* launcherDir,
                                    const char* gpu) {
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

    if (gpu && strstr(gpu, "angle") != NULL) {
        bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle");
        D("Adding library search path: '%s'", fullPath);
        add_library_search_dir(fullPath);

        bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle9");
        D("Adding library search path: '%s'", fullPath);
        add_library_search_dir(fullPath);

        bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_angle11");
        D("Adding library search path: '%s'", fullPath);
        add_library_search_dir(fullPath);
    } else  {
        // We add this last so Win32 can resolve LIBGLESV2 from swiftshader for QT5GUI
        bufprint(fullPath, fullPath + sizeof(fullPath), "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir, "gles_swiftshader");
        D("Adding library search path: '%s'", fullPath);
        add_library_search_dir(fullPath);
    }

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

#if defined(__aarch64__)
    if (isHeadless) {
        // for headless mode on linux, uses stub xlib
        bufprint(fullPath, fullPath + sizeof(fullPath),
                 "%s" PATH_SEP "%s" PATH_SEP "%s", launcherDir, libSubDir,
                 "libStubXlib.so");
        D("Preload stub xlib: %s", fullPath);
        appendPreloadLib(fullPath);
    } else {
        // not headless, append pulse sound if it exists
        const char* pulse_lib_path = "/usr/lib/aarch64-linux-gnu/libpulse.so.0";
        if (path_exists(pulse_lib_path)) {
            D("Preload pulse lib %s", pulse_lib_path);
            appendPreloadLib(pulse_lib_path);
        }
        // do not use glib in qt
        System::get()->envSet("QT_NO_GLIB", "1");
    }
#else   // !__linux__
    (void)isHeadless;
#endif  // !__linux__

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
