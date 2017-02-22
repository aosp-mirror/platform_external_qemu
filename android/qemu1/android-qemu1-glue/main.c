/* Copyright (C) 2006-2016 The Android Open Source Project
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

#include "android/android.h"
#include "android/crashreport/crash-handler.h"
#include "android/featurecontrol/feature_control.h"
#include "android/globals.h"
#include "android/hw-kmsg.h"
#include "android/main-common-ui.h"
#include "android/main-common.h"
#include "android/main-kernel-parameters.h"
#include "android/main-qemu-parameters.h"
#include "android/process_setup.h"
#include "android/skin/winsys.h"
#include "android/utils/aconfig-file.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/lineinput.h"

#include "android-qemu1-glue/emulation/serial_line.h"
#include "android-qemu1-glue/qemu-control-impl.h"
#ifdef __APPLE__
#include "android-qemu1-glue/skin_qt.h"
#endif

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

int qemu_main(int argc, char **argv);

void enter_qemu_main_loop(int argc, char **argv) {
#ifndef _WIN32
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif

    D("Starting QEMU main loop");
    qemu_main(argc, argv);
    D("Done with QEMU main loop");
}

#if defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain().
#define main qt_main
#endif

int main(int argc, char **argv) {
    char* args[128];
    AndroidHwConfig* hw = android_hw;
    AvdInfo* avd;
    int exitStatus = 1;

    AndroidOptions  opts[1] = {};

    process_early_setup(argc, argv);

    const char* argv0 = argv[0];

    static const char kTargetArch[] =
#if defined(TARGET_ARM)
        "arm"
#elif defined(TARGET_I386)
        "x86"
#elif defined(TARGET_X86_64)
        "x86_64"
#elif defined(TARGET_MIPS)
        "mips"
#elif defined(TARGET_MIPS64)
        "mips64"
#else
#error "Unknown target CPU architecture"
#endif
        ;

        if (!emulator_parseCommonCommandLineOptions(&argc,
                                                    &argv,
                                                    kTargetArch,
                                                    false,  // is_qemu2
                                                    opts,
                                                    hw,
                                                    &android_avdInfo,
                                                    &exitStatus)) {
            // Special case for QEMU positional parameters.
            if (exitStatus == EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER) {
                // Copy all QEMU options to |args|, and set |n| to the number
                // of options in |args| (|argc| must be positive here).
                int n;
                for (n = 1; n <= argc; ++n) {
                    args[n] = argv[n - 1];
                }

                // Skip the translation of command-line options and jump
                // straight to qemu_main().
                enter_qemu_main_loop(n, args);
                return 0;
            }

            // Normal exit.
            return exitStatus;
        }

    // Update server-based hw config / feature flags.
    // Must be done after emulator_parseCommonCommandLineOptions,
    // since that calls createAVD which sets up critical info needed
    // by featurecontrol component itself.
    feature_update_from_server();

    // we know it's qemu1, and don't care what user wanted to trick us into
    opts->ranchu = 0;

    avd = android_avdInfo;

    if (!emulator_parseUiCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    // Overide default null SerialLine implementation
    // before android_kmsg_init()
    qemu1_android_serialline_init();

    if (opts->show_kernel) {
      // Ensure kernel messages are printed to stdout.
      android_kmsg_init(ANDROID_KMSG_PRINT_MESSAGES);
    }

    const char* serialPrefix = androidHwConfig_getKernelSerialPrefix(hw);
    AndroidGlesEmulationMode glesMode = kAndroidGlesEmulationOff;

    if (!strcmp(android_hw->hw_gpu_mode, "guest")) {
        glesMode = kAndroidGlesEmulationGuest;
    } else if (android_hw->hw_gpu_enabled) {
        glesMode = kAndroidGlesEmulationHost;
    }

    int apiLevel = avdInfo_getApiLevel(avd);

    char* kernelParameters = emulator_getKernelParameters(
        opts, kTargetArch, apiLevel, serialPrefix, hw->kernel_parameters,
        glesMode, 0ULL, false);

    if (hw->hw_cpu_ncore > 1) {
        // Avoid printing this warning all the time because the default
        // value for this hardware property is now 2, meaning that any
        // new AVD created by the user will trigger this condition, even
        // though they didn't specifically ask for it. As a rule of thumb,
        // it's always better to print a warning when it corresponds to
        // a user-selectable condition, not a default.
        if (VERBOSE_CHECK(init)) {
            dwarning("Classic qemu does not support SMP. "
                     "The hw.cpu.ncore option from your config file is ignored.");
        }
    }

    /* Generate a hardware-qemu.ini for this AVD. The real hardware
     * configuration is ususally stored in several files, e.g. the AVD's
     * config.ini plus the skin-specific hardware.ini.
     *
     * The new file will group all definitions and will be used to
     * launch the core with the -android-hw <file> option.
     */
    const char* coreHwIniPath = avdInfo_getCoreHwIniPath(avd);
    {
        CIniFile* hwIni = iniFile_newEmpty(NULL);
        androidHwConfig_write(hw, hwIni);

        if (filelock_create(coreHwIniPath) == NULL) {
            // The AVD is already in use
            derror("There's another emulator instance running with "
                   "the current AVD '%s'. Exiting...\n", avdInfo_getName(avd));
            return 1;
        }

        /* While saving HW config, ignore valueless entries. This will not break
         * anything, but will significantly simplify comparing the current HW
         * config with the one that has been associated with a snapshot (in case
         * VM starts from a snapshot for this instance of emulator). */
        if (iniFile_saveToFileClean(hwIni, coreHwIniPath) < 0) {
            derror("Could not write hardware.ini to %s: %s", coreHwIniPath, strerror(errno));
            return 2;
        }

        crashhandler_copy_attachment(CRASH_AVD_HARDWARE_INFO, coreHwIniPath);

        /* In verbose mode, dump the file's content */
        if (VERBOSE_CHECK(init)) {
            FILE* file = fopen(coreHwIniPath, "rt");
            if (file == NULL) {
                derror("Could not open hardware configuration file: %s\n",
                       coreHwIniPath);
            } else {
                LineInput* input = lineInput_newFromStdFile(file);
                const char* line;
                printf("Content of hardware configuration file:\n");
                while ((line = lineInput_getLine(input)) !=  NULL) {
                    printf("  %s\n", line);
                }
                printf(".\n");
                lineInput_free(input);
                fclose(file);
            }
        }
    }

    // NOTE: |argc| and |argv| have been adjusted by the
    // emulator_parseCommonCommandLineOptions() function and now
    // corresponds to the parameters following -qemu, if any.
    QemuParameters* qemuParams = qemu_parameters_create(
        argv0, argc, (const char* const*)argv, opts, avd, kernelParameters,
        coreHwIniPath, false,  // is_qemu2
        kTargetArch);
    if (!qemuParams) {
        return 1;
    }

    static UiEmuAgent uiEmuAgent;
    uiEmuAgent.battery = gQAndroidBatteryAgent;
    uiEmuAgent.cellular = gQAndroidCellularAgent;
    uiEmuAgent.finger = gQAndroidFingerAgent;
    uiEmuAgent.location = gQAndroidLocationAgent;
    uiEmuAgent.sensors = gQAndroidSensorsAgent;
    uiEmuAgent.telephony = gQAndroidTelephonyAgent;
    uiEmuAgent.userEvents = gQAndroidUserEventAgent;
    uiEmuAgent.window = gQAndroidEmulatorWindowAgent;
    uiEmuAgent.car = gQCarDataAgent;

    // for now there's no uses of SettingsAgent, so we don't set it
    uiEmuAgent.settings = NULL;

    /* Setup SDL UI just before calling the code */
#ifdef __linux__
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif
    skin_winsys_init_args(argc, argv);

    if (!emulator_initUserInterface(opts, &uiEmuAgent)) {
        return 1;
    }

    // This is a workaround for b.android.com/198256
    // Qemu1 QT GUI on OSX crashes on exit when QT releases NSWindow.
    // Qemu2 builds do not show this crash behavior.
    // Workaround leaks the EmulatorQtWindow instance, which was the undesired
    // behavior prior to https://android-review.googlesource.com/#/c/199068/
    // Root cause has not been identified
#ifdef __APPLE__
    skin_acquire_window_inst();
#endif
    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop,
                             qemu_parameters_size(qemuParams),
                             qemu_parameters_array(qemuParams));

    skin_winsys_enter_main_loop(opts->no_window);

    emulator_finiUserInterface();

    qemu_parameters_free(qemuParams);

    process_late_teardown();
    return 0;
}
