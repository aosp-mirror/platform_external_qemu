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

// Parse command-line options and setups |opt| and |hw| structures.
// |p_argc| and |p_argv| are pointers to the command-line parameters
// received from main(). |targetArch| is the target architecture for
// platform build. |opt| and |hw| are caller-provided structures
// that will be initialized by the function. |*the_avd| will receive
// a pointer to a new AvdInfo if the function returns +1.
//
// The return value is an integer with several meanings:
//   +2 : A QEMU positional parameter was detected. The caller
//        should copy all arguments from |*p_argc| and |*p_argv|
//        and call the QEMU main function with them, then exit.
//
//   +1 : Normal emulation start.
//
//    n, with n <= 0 : Exit program immediately with status code -n.
//
int handleEmulatorCommandLine(int* p_argc,
                              char*** p_argv,
                              const char* targetArch,
                              AndroidOptions* opt,
                              AndroidHwConfig* hw,
                              AvdInfo** the_avd);

ANDROID_END_HEADER
