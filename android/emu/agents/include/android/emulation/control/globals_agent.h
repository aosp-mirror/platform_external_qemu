/* Copyright (C) 2022 The Android Open Source Project
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
#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include "android/utils/compiler.h"

/** TODO:(jansene): These are the set of global variables that are used
 *throughout the system. Most of these are not yet available and commented out.
 *
 **/

ANDROID_BEGIN_HEADER

typedef struct LanguageSettings {
    int changing_language_country_locale;
    char* language;
    char* country;
    char* locale;
} LanguageSettings;

typedef struct AvdInfoParams AvdInfoParams;
typedef struct AvdInfo AvdInfo;
typedef struct AndroidHwConfig AndroidHwConfig;
typedef struct AUserConfig AUserConfig;
typedef struct AndroidOptions AndroidOptions;

typedef struct QAndroidGlobalVarsAgent {
    /* this structure is setup when loading the virtual device
     * after that, you can read the 'flags' field to determine
     * wether a data or cache wipe has been in effect.
     */
    AvdInfoParams* (*avdParams)(void);

    /* a pointer to the android virtual device information
     * object, which can be queried for the paths of various
     * image files or the skin
     */
    AvdInfo* (*avdInfo)(void);

    // /* MSVC only exports function pointers */
    // AvdInfo** (*aemu_get_getConsoleAgents()->settings->avdInfo()Ptr)(void);

    // /* the hardware configuration for this specific virtual device */
    AndroidHwConfig* (*hw)(void);

    // /* this indicates that guest has mounted data partition */
    int (*guest_data_partition_mounted)(void);

    // /* this indicates that guest has boot completed */
    bool (*guest_boot_completed)(void);

    bool (*arm_snapshot_save_completed)(void);

    bool (*host_emulator_is_headless)(void);

    // /* are we using the emulator in the android mode or plain qemu? */
    bool (*android_qemu_mode)(void);

    // /* are we using android-emu libraries for a minimal configuration? */
    bool (*min_config_qemu_mode)(void);

    // /* is android-emu running Fuchsia? */
    bool (*is_fuchsia)(void);

    int (*android_snapshot_update_timer)(void);

    LanguageSettings* (*language)(void);

    // /* True if we are using keycode forwarding instead of translating text
    // value
    //  * to keycode */
    // /* on emulator host. */
    bool (*use_keycode_forwarding)(void);

    AUserConfig* (*userConfig)(void);

    // The parsed command line options.
    AndroidOptions* (*android_cmdLineOptions)(void);

    // Injects the cmdLineOptions, making them available to everyone.
    // You **SHOULD** not really need to call this.
    void (*inject_cmdLineOptions)(AndroidOptions*);

    // TRUE if the commandline options have been injected (You usually do not
    // need to call this)
    bool (*has_cmdLineOptions)(void);

    // The commandline used to start the emulator.
    const char* (*android_cmdLine)(void);

    // Injects the cmdLine, making it available to everyone.
    // You **SHOULD** not really need to call this.
    void (*inject_android_cmdLine)(const char*);

    // Injects the language settings, this will set the
    // changing_language_country_locale variable if any of the parameters are
    // not null.
    void (*inject_language)(char* /*language*/,
                            char* /*country*/,
                            char* /*locale*/);

    void (*inject_userConfig)(AUserConfig* /*config*/);
    void (*set_keycode_forwarding)(bool /*enabled */);
    void (*inject_AvdInfo)(AvdInfo*);

    // /* this indicates that guest has mounted data partition */
    void (*set_guest_data_partition_mounted)(int);

    // /* this indicates that guest has boot completed */
    void (*set_guest_boot_completed)(bool);

    void (*set_arm_snapshot_save_completed)(bool);

    void (*set_host_emulator_is_headless)(bool);

    // /* are we using the emulator in the android mode or plain qemu? */
    void (*set_android_qemu_mode)(bool);

    // /* are we using android-emu libraries for a minimal configuration? */
    void (*set_min_config_qemu_mode)(bool);

    // /* is android-emu running Fuchsia? */
    void (*set_is_fuchsia)(bool);

    void (*set_android_snapshot_update_timer)(int);

    /* this is the port used for the control console in this emulator instance.
     * starts at 5554, with increments of 2 */
    int (*android_base_port)(void);
    void (*set_android_base_port)(int);

    /* this is the port used to connect ADB (Android Debug Bridge)
     * default is 5037 */
    int (*android_adb_port)(void);
    void (*set_android_adb_port)(int);

    /* This number is used to form the "serial number" of
     * the AVD. The default is 5554, so the default serial
     * number is "emulator-5554". */
    int (*android_serial_number_port)(void);
    void (*set_android_serial_number_port)(int);

    bool (*has_network_option)(void);
} QAndroidGlobalVarsAgent;
ANDROID_END_HEADER
