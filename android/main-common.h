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

#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/cmdline-option.h"
#include "android/utils/aconfig-file.h"
#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

/* Common routines used by both android-qemu1-glue/main.c and android/main-ui.c */

// Reset the value of |*string| to a copy of |new_value|. This
// will free() the previous value of |*string| first.
void reassign_string(char** string, const char* new_value);

unsigned convertBytesToMB( uint64_t  size );
uint64_t convertMBToBytes( unsigned  megaBytes );

#define NETWORK_SPEED_DEFAULT  "full"
#define NETWORK_DELAY_DEFAULT  "none"

extern const char*  skin_network_speed;
extern const char*  skin_network_delay;

/* Sanitize options. This deals with a few legacy options that are now
 * handled differently. Call before anything else that needs to read
 * the options list.
 */
void sanitizeOptions( AndroidOptions* opts );

/* Creates and initializes AvdInfo instance for the given options.
 * Param:
 *  opts - Options passed to the main()
 *  inAndroidBuild - Upon exit contains 0 if AvdInfo has been initialized from
 *      AVD file, or 1 if AvdInfo has been initialized from the build directory.
 * Return:
 *  AvdInfo instance initialized for the given options.
 */
struct AvdInfo* createAVD(AndroidOptions* opts, int* inAndroidBuild);

/* Handle the command-line options that are common to both the classic and
 * and new emulator code bases. |opts| is the set of options, that may be
 * modified by the function, |hw| is the hardware configuration that will
 * be modified by the function, and |avd| is the AVD information data.
 */
void handleCommonEmulatorOptions(AndroidOptions* opts,
                                 AndroidHwConfig* hw,
                                 AvdInfo* avd);

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
