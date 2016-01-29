/* Copyright (C) 2006-2015 The Android Open Source Project
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

#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif
#ifdef _WIN32
#include <process.h>
#endif

#if defined(__APPLE__) && defined(CONFIG_SDL)
// This include is currently required to ensure that 'main' is renamed
// to 'SDL_main' as a macro. This is required by SDL 1.x on OS X.
#include "SDL.h"
#endif  // __APPLE__

#include "config.h"

#include "android/android.h"
#include "qemu-common.h"
#include "sysemu/sysemu.h"
#include "ui/console.h"

#include "math.h"

#include "android/config/config.h"

#include "android/kernel/kernel_utils.h"
#include "android/skin/charmap.h"
#include "android/user-config.h"

#include "android/utils/aconfig-file.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/lineinput.h"
#include "android/utils/path.h"
#include "android/utils/property_file.h"
#include "android/utils/tempfile.h"
#include "android/utils/sockets.h"

#include "android/main-common.h"
#include "android/main-common-ui.h"
#include "android/help.h"
#include "hw/android/goldfish/nand.h"

#include "android/globals.h"

#include "android-qemu1-glue/display.h"
#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/snapshot.h"

#include "android/framebuffer.h"
#include "android/opengl/emugl_config.h"
#include "android/utils/iolooper.h"

#include "android/skin/winsys.h"
#include "android/version.h"

SkinRotation  android_framebuffer_rotation;

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

extern int  control_console_start( int  port );  /* in control.c */

extern int qemu_milli_needed;

/* the default device DPI if none is specified by the skin
 */
#define  DEFAULT_DEVICE_DPI  165

int qemu_main(int argc, char **argv);

/* this function dumps the QEMU help */
extern void  help( void );
extern void  emulator_help( void );

#define  VERBOSE_OPT(str,var)   { str, &var }

#define  _VERBOSE_TAG(x,y)   { #x, VERBOSE_##x, y },

void emulator_help( void )
{
    STRALLOC_DEFINE(out);
    android_help_main(out);
    printf( "%.*s", out->n, out->s );
    stralloc_reset(out);
}

/*
 * _findQemuInformationalOption: search for informational QEMU options
 *
 * Scans the given command-line options for any informational QEMU option (see
 * |qemu_info_opts| for the list of informational QEMU options). Returns the
 * first matching option, or NULL if no match is found.
 *
 * |qemu_argc| is the number of command-line options in |qemu_argv|.
 * |qemu_argv| is the array of command-line options to be searched. It is the
 * caller's responsibility to ensure that all these options are intended for
 * QEMU.
 */
static char*
_findQemuInformationalOption( int qemu_argc, char** qemu_argv )
{
    /* Informational QEMU options, which make QEMU print some information to the
     * console and exit. */
    static const char* const qemu_info_opts[] = {
        "-h",
        "-help",
        "-version",
        "-audio-help",
        "?",           /* e.g. '-cpu ?' for listing available CPU models */
        NULL           /* denotes the end of the list */
    };
    int i = 0;

    for (; i < qemu_argc; i++) {
        char* arg = qemu_argv[i];
        const char* const* oo = qemu_info_opts;

        for (; *oo; oo++) {
            if (!strcmp(*oo, arg)) {
                return arg;
            }
        }
    }
    return NULL;
}

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

/*
 * Return true if software GPU is used and AVD screen is too large for it.
 * Software GPU can boot 768 X 1280 (Nexus 4) or smaller due to software
 * buffer size. (It may actually boot a slightly larger screen, but we set
 * limit to this commonly seen resolution.)
 */
static bool use_software_gpu_and_screen_too_large(AndroidHwConfig *hw) {
    return (!hw->hw_gpu_enabled &&
            hw->hw_lcd_width * hw->hw_lcd_height > 768 * 1280);
}

#if defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain().
#define main qt_main
#endif

int main(int argc, char **argv) {
    char*  opt;

    AndroidHwConfig*  hw;
    AvdInfo*          avd;
    AConfig*          skinConfig;
    char*             skinPath;
    int               inAndroidBuild;

    AndroidOptions  opts[1] = {};

    if ( android_parse_options( &argc, &argv, opts ) < 0 ) {
        return 1;
    }

    /* Parses options and builds an appropriate AVD. */
    avd = android_avdInfo = createAVD(opts, &inAndroidBuild);
        char* skinName;
        char* skinDir;

        avdInfo_getSkinInfo(avd, &skinName, &skinDir);

        if (opts->skin == NULL) {
            opts->skin = skinName;
            D("autoconfig: -skin %s", opts->skin);
        } else {
            AFREE(skinName);
        }

        opts->skindir = skinDir;
        D("autoconfig: -skindir %s", opts->skindir);

    /* update the avd hw config from this new skin */
    avdInfo_getSkinHardwareIni(avd, opts->skin, opts->skindir);

    /* Read hardware configuration */
    hw = android_hw;
    if (avdInfo_initHwConfig(avd, hw) < 0) {
        derror("could not read hardware configuration ?");
        return 1;
    }

    parse_skin_files(opts->skindir, opts->skin, opts, hw,
                     &skinConfig, &skinPath);

    if (!opts->netspeed && skin_network_speed) {
        D("skin network speed: '%s'", skin_network_speed);
        if (strcmp(skin_network_speed, NETWORK_SPEED_DEFAULT) != 0) {
            opts->netspeed = (char*)skin_network_speed;
        }
    }
    if (!opts->netdelay && skin_network_delay) {
        D("skin network delay: '%s'", skin_network_delay);
        if (strcmp(skin_network_delay, NETWORK_DELAY_DEFAULT) != 0) {
            opts->netdelay = (char*)skin_network_delay;
        }
    }

    if (opts->code_profile) {
        char*   profilePath = avdInfo_getCodeProfilePath(avd, opts->code_profile);
        int     ret;

        if (profilePath == NULL) {
            derror( "bad -code-profile parameter" );
            return 1;
        }
    }

    if (hw->vm_heapSize == 0) {
            printf ("HELLO\n");
    }

    volatile int buf [800];
    for (int i = 0; i < 800; i ++) {
        buf[i]+=1;
    }

    static UiEmuAgent uiEmuAgent;

    ui_init(skinConfig, skinPath, opts, &uiEmuAgent);
//    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop, n, args);
    skin_winsys_enter_main_loop(opts->no_window, argc, argv);
    ui_done();

    return 0;
}
