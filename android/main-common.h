/* Copyright (C) 2015 The Android Open Source Project
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

#include <stdint.h>
#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/cmdline-option.h"
#include "android/skin/keyset.h"
#include "android/ui-emu-agent.h"
#include "android/utils/aconfig-file.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* Common routines used by both android-qemu1-glue/main.c and android/main-ui.c */

// Reset the value of |*string| to a copy of |new_value|. This
// will free() the previous value of |*string| first.
void reassign_string(char** string, const char* new_value);

unsigned convertBytesToMB( uint64_t  size );
uint64_t convertMBToBytes( unsigned  megaBytes );

extern SkinKeyset*  android_keyset;
void parse_keyset(const char*  keyset, AndroidOptions*  opts);
void write_default_keyset( void );

#define NETWORK_SPEED_DEFAULT  "full"
#define NETWORK_DELAY_DEFAULT  "none"

extern const char*  skin_network_speed;
extern const char*  skin_network_delay;

/* Returns the amount of pixels used by the default display. */
int64_t  get_screen_pixels(AConfig*  skinConfig);

int init_sdl_ui(AndroidOptions* opts,
                AndroidHwConfig* hw,
                const UiEmuAgent* uiEmuAgent);

/* Creates and initializes AvdInfo instance for the given options.
 * Param:
 *  opts - Options passed to the main()
 *  inAndroidBuild - Upon exit contains 0 if AvdInfo has been initialized from
 *      AVD file, or 1 if AvdInfo has been initialized from the build directory.
 * Return:
 *  AvdInfo instance initialized for the given options.
 */
struct AvdInfo* createAVD(AndroidOptions* opts, int* inAndroidBuild);

// Parse command-line options specified by |*argc| and |*argv| and extract
// useful information. On success, sets up the content of |opts| and |hwConfig|
// and creates a new AvdInfo instance whose address is returned in |*avd|,
// a new AConfig in |*skinConfig| na d a new skin path in |*skinPath|.
//
// The return value indicates failure or exit conditions in different
// ways:
//
//     1 -> success, continue to launch the emulator.
//     2 -> informational QEMU options detected, just pass the new values
//          of |*argc| and |*argv| to the QEMU main function to let it
//          display an informative text and exit.
//     0 -> exit process immediately with status 0 (used to implement -help)
//    -n -> for |n >= 1|, exit process immediately with status |-n|
//
// IMPORTANT: Even in case of failure, the caller is responsible for
// freeing |*avd| if it is not null.
int parseEmulatorCommandLineOptions(int* argc,
                                    char*** argv,
                                    AndroidOptions* opts,
                                    AndroidHwConfig* hwConfig,
                                    AvdInfo** avd);

/* Populate the hwConfig fields corresponding to the kernel/disk images
 * used by the emulator. This will zero *hwConfig first.
 */
void findImagePaths( AndroidHwConfig*  hwConfig,
                     AndroidOptions*   opts );

/* Updates hardware configuration for the given AVD and options.
 * Param:
 *  hwConfig - Hardware configuration to update.
 *  avd - AVD info containig paths for the hardware configuration.
 *  opts - Options passed to the main()
 *  inAndroidBuild - 0 if AVD has been initialized from AVD file, or 1 if AVD
 *      has been initialized from the build directory.
 */
void updateHwConfigFromAVD(AndroidHwConfig* hwConfig, struct AvdInfo* avd,
                           AndroidOptions* opts, int inAndroidBuild);

typedef enum {
    ACCEL_OFF = 0,
    ACCEL_ON = 1,
    ACCEL_AUTO = 2,
} CpuAccelMode;

#ifdef __linux__
    static const char kAccelerator[] = "KVM";
    static const char kEnableAccelerator[] = "-enable-kvm";
    static const char kDisableAccelerator[] = "-disable-kvm";
#else
    static const char kAccelerator[] = "Intel HAXM";
    static const char kEnableAccelerator[] = "-enable-hax";
    static const char kDisableAccelerator[] = "-disable-hax";
#endif

/*
 * Param:
 *  opts - Options passed to the main()
 *  avd - AVD info containig paths for the hardware configuration.
 *  accel_mode - indicates acceleration mode based on command line
 *  status - a string about cpu acceleration status
 * Return: if cpu acceleration is available
 */
bool handleCpuAcceleration(AndroidOptions* opts, AvdInfo* avd,
                           CpuAccelMode* accel_mode, char* accel_status);

ANDROID_END_HEADER
