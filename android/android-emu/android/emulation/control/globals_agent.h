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
#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/base/export.h"
#include "android/cmdline-definitions.h"
#include "android/constants.h"
#include "android/user-config.h"
#include "android/utils/compiler.h"
#include <stdbool.h>

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
    // int (*guest_data_partition_mounted)(void);

    // /* this indicates that guest has boot completed */
    // int (*guest_boot_completed)(void);

    // int (*arm_snapshot_save_completed)(void);

    // int (*host_emulator_is_headless)(void);

    // /* are we using the emulator in the android mode or plain qemu? */
    // int (*android_qemu_mode)(void);

    // /* are we using android-emu libraries for a minimal configuration? */
    // int (*min_config_qemu_mode)(void);

    // /* is android-emu running Fuchsia? */
    // int (*is_fuchsia)(void);

    // int (*android_snapshot_update_timer)(void);

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

    // Injects the language settings, this will set the changing_language_country_locale
    // variable if any of the parameters are not null.
    void (*inject_language)(char* /*language*/, char* /*country*/, char* /*locale*/);

    void (*inject_userConfig)(AUserConfig* /*config*/);
    void (*set_keycode_forwading)(bool /*enabled */);
    void (*inject_AvdInfo)(AvdInfo*);
} QAndroidGlobalVarsAgent;
ANDROID_END_HEADER
