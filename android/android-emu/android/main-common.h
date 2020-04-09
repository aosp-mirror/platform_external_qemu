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
#include "android/cpu_accelerator.h"
#include "android/cmdline-option.h"
#include "android/emulation/control/vm_operations.h"
#include "android/opengl/emugl_config.h"
#include "android/skin/winsys.h"
#include "android/utils/aconfig-file.h"
#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

// Special value return
#define EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER  (-1)

// Parse command-line options and setups |opt| and |hw| structures.
// |p_argc| and |p_argv| are pointers to the command-line parameters
// received from main(). |targetArch| is the target architecture for
// platform builds. |is_qemu2| is true if this is called from QEMU2,
// false if called from the classic emulator. |opt| and |hw| are
// caller-provided structures that will be initialized by the function.
//
// On success, return true and sets |*the_avd| to the address of a new
// AvdInfo instance. On failure, return false and sets |*exit_status|
// to a process exit status.
//
// NOTE: As a special case |*exit_status| will be set to
// EMLATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER on failure to indicate that
// a QEMU positional parameter was detected. The caller should copy all
// arguments from |*p_argc| and |*p_argv| and call the QEMU main function
// with them, then exit.
bool emulator_parseCommonCommandLineOptions(int* p_argc,
                                            char*** p_argv,
                                            const char* targetArch,
                                            bool is_qemu2,
                                            AndroidOptions* opt,
                                            AndroidHwConfig* hw,
                                            AvdInfo** the_avd,
                                            int* exit_status);

// Handle command-line options and AVD configurations that are dependent on
// the status of advanced features.  This function will update the content of
// |hw| based on the values found in |opts| and |avd|, so call this before
// writing hardware-qemu.img to disk. Return true on success, false otherwise.
bool emulator_parseFeatureCommandLineOptions(AndroidOptions* opts,
                                             AvdInfo* avd,
                                             AndroidHwConfig* hw);

// configAndStartRenderer():
// Perform renderer configuration based on the host GPU
// (or guest renderer if applicable), and then start the renderer.
// The renderer needs to be started in order to query OpenGL
// capabilities.
// Outputs:
// |glesMode|: Resolved rendering mode (on host or guest)
// |opengl_alive|: Tracks whether or not the renderer startup failed.
// |bootPropOpenglesVersion|; Constructed ro.opengles.version boot property
// appropriate to the detected OpenGL ES API level support.
// |glFramebufferSizeBytes|: Returns size of LCD-bound framebuffer in bytes.
// |selectedRenderer|: Returns OpenGL ES backend (host/angle/swiftshader etc)
// Useful for reserving extra memory for in-guest framebuffers.

// First, there is a struct to hold outputs.
typedef struct {
    AndroidGlesEmulationMode glesMode;
    int openglAlive;
    int bootPropOpenglesVersion;
    int glFramebufferSizeBytes;
    SelectedRenderer selectedRenderer;
} RendererConfig;
// Function itself:

bool configAndStartRenderer(
         AvdInfo* avd, AndroidOptions* opt, AndroidHwConfig* hw,
         const struct QAndroidVmOperations *vm_operations,
         const struct QAndroidEmulatorWindowAgent *window_agent,
         enum WinsysPreferredGlesBackend uiPreferredBackend,
         RendererConfig* config_out);

// stopRenderer() - stop all the render threads and wait until their exit.
// NOTE: It is only safe to stop the OpenGL ES renderer after the main loop
//   has exited. This is not necessarily before |skin_window_free| is called,
//   especially on Windows!
void stopRenderer(void);

// After configAndStartRenderer is called, one can query last output values:
RendererConfig getLastRendererConfig(void);

// HACK: Value will be true if emulator_parseCommonCommandLineOptions()
//       has seen a network-related option (e.g. -netspeed). This is
//       unfortunately used by the Qt UI code.
//
// TODO: Find a better way to deal with this.
extern bool emulator_has_network_option;

/* Common routines used by both android-qemu1-glue/main.c and android/main-ui.c */

unsigned convertBytesToMB( uint64_t  size );
uint64_t convertMBToBytes( unsigned  megaBytes );

#define NETWORK_SPEED_DEFAULT  "full"
#define NETWORK_DELAY_DEFAULT  "none"

extern const char*  android_skin_net_speed;
extern const char*  android_skin_net_delay;

typedef enum {
    ACCEL_OFF = 0,
    ACCEL_ON = 1,
    ACCEL_AUTO = 2,
    ACCEL_KVM = 3,
    ACCEL_HAX = 4,
    ACCEL_HVF = 5,
    ACCEL_WHPX = 6,
} CpuAccelMode;

// TODO: the accelerator should not be assumed only based on the
// platform. More than one may be available for each platform.
#ifdef __linux__
    static const char kAccelerator[] = "KVM";
    static const char kEnableAccelerator[] = "-enable-kvm";
    static const char kDisableAccelerator[] = "-disable-kvm";
#else
#ifdef _WIN32
    static const char kAccelerator[] = "Windows Hypervisor Platform (WHPX)";
    static const char kEnableAccelerator[] = "-enable-whpx";
    static const char kDisableAccelerator[] = "-disable-whpx";
    static const char kAcceleratorHAX[] = "Intel HAXM";
    static const char kEnableAcceleratorHAX[] = "-enable-hax";
    static const char kDisableAcceleratorHAX[] = "-disable-hax";
#else
    static const char kAccelerator[] = "Intel HAXM";
    static const char kEnableAccelerator[] = "-enable-hax";
    static const char kDisableAccelerator[] = "-disable-hax";
    static const char kAcceleratorHVF[] = "Apple Hypervisor.framework";
    static const char kEnableAcceleratorHVF[] = "-enable-hvf";
    static const char kDisableAcceleratorHVF[] = "-disable-hvf";
#endif
#endif

/*
 * Param:
 *  opts - Options passed to the main()
 *  avd - AVD info containig paths for the hardware configuration.
 *  accel_mode - indicates acceleration mode based on command line
 *  status - a string about cpu acceleration status, must be not null.
 * Return: if cpu acceleration is available
 */
bool handleCpuAcceleration(AndroidOptions* opts, const AvdInfo* avd,
                           CpuAccelMode* accel_mode, char** accel_status);

const char* getAcceleratorEnableParam(AndroidCpuAccelerator accel_type);

ANDROID_END_HEADER
