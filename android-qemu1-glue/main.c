/* Copyright (C) 2006-2008 The Android Open Source Project
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
    exit(1);
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

#if defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain().
#define main qt_main
#endif

int main(int argc, char **argv) {
    char*  args[128];
    int    n;

    /* The emulator always uses the first serial port for kernel messages
     * and the second one for qemud. So start at the third if we need one
     * for logcat or 'shell'
     */
    int    serial = 2;
    int    shell_serial = 0;


    args[0] = argv[0];

    AndroidOptions opts[1];
    AndroidHwConfig* hw = android_hw;
    AvdInfo* avd = NULL;

    int ret = parseEmulatorCommandLineOptions(&argc, &argv, opts, hw, &avd);
    if (ret <= 0) {
        if (avd) {
            avdInfo_free(avd);
        }
        exit(-ret);
    }

    android_avdInfo = avd;

#ifdef _WIN32
    socket_init();
#endif

    if (ret == 2) {
        // QEMU informational option was found.
        n = 1;
        do {
            args[n] = argv[n - 1];
        } while (n++ < argc);
        args[n] = NULL;

        /* Skip the translation of command-line options and jump straight to
         * qemu_main(). */
        enter_qemu_main_loop(n, args);
        return 0;
    }

    /* Update CPU architecture for HW configs created from build dir. */
    if (avdInfo_inAndroidBuild(avd)) {
#if defined(TARGET_ARM)
        reassign_string(&android_hw->hw_cpu_arch, "arm");
#elif defined(TARGET_I386)
        reassign_string(&android_hw->hw_cpu_arch, "x86");
#elif defined(TARGET_MIPS)
        reassign_string(&android_hw->hw_cpu_arch, "mips");
#elif defined(TARGET_MIPS64)
        reassign_string(&android_hw->hw_cpu_arch, "mips64");
#endif
    }

    n = 1;

    if (opts->shared_net_id) {
        /* net.shared_net_ip boot property value. */
        char boot_prop_ip[64];
        boot_prop_ip[0] = '\0';

        char*  end;
        long   shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            fprintf(stderr, "option -shared-net-id must be an integer between 1 and 255\n");
            exit(1);
        }
        snprintf(boot_prop_ip, sizeof(boot_prop_ip),
                 "net.shared_net_ip=10.1.2.%ld", shared_net_id);

        args[n++] = "-boot-property";
        args[n++] = ASTRDUP(boot_prop_ip);
    }

    if (opts->tcpdump) {
        args[n++] = "-tcpdump";
        args[n++] = opts->tcpdump;
    }

#ifdef CONFIG_NAND_LIMITS
    if (opts->nand_limits) {
        args[n++] = "-nand-limits";
        args[n++] = opts->nand_limits;
    }
#endif

    if (opts->timezone) {
        args[n++] = "-timezone";
        args[n++] = opts->timezone;
    }

    if (opts->netspeed) {
        args[n++] = "-netspeed";
        args[n++] = opts->netspeed;
    }
    if (opts->netdelay) {
        args[n++] = "-netdelay";
        args[n++] = opts->netdelay;
    }
    if (opts->netfast) {
        args[n++] = "-netfast";
    }

    if (opts->audio) {
        args[n++] = "-audio";
        args[n++] = opts->audio;
    }

    if (opts->cpu_delay) {
        args[n++] = "-cpu-delay";
        args[n++] = opts->cpu_delay;
    }

    if (opts->dns_server) {
        args[n++] = "-dns-server";
        args[n++] = opts->dns_server;
    }

    /* If we have a valid snapshot storage path */
    if (opts->snapstorage) {
        /* We still use QEMU command-line options for the following since
        * they can change from one invokation to the next and don't really
        * correspond to the hardware configuration itself.
        */
        if (!opts->no_snapshot_load) {
            args[n++] = "-loadvm";
            args[n++] = ASTRDUP(opts->snapshot);
        }

        if (!opts->no_snapshot_save) {
            args[n++] = "-savevm-on-exit";
            args[n++] = ASTRDUP(opts->snapshot);
        }

        if (opts->no_snapshot_update_time) {
            args[n++] = "-snapshot-no-time-update";
        }
    }

    /* we always send the kernel messages from ttyS0 to android_kmsg */
    if (opts->show_kernel) {
        args[n++] = "-show-kernel";
    }

    if (opts->shell || opts->logcat) {
        args[n++] = "-serial";
        args[n++] = opts->shell_serial;
        shell_serial = serial++;
    }

    if (opts->radio) {
        args[n++] = "-radio";
        args[n++] = opts->radio;
    }

    if (opts->gps) {
        args[n++] = "-gps";
        args[n++] = opts->gps;
    }

    if (opts->code_profile) {
        char*   profilePath = avdInfo_getCodeProfilePath(avd, opts->code_profile);
        int     ret;

        if (profilePath == NULL) {
            derror( "bad -code-profile parameter" );
            exit(1);
        }
        ret = path_mkdir_if_needed( profilePath, 0755 );
        if (ret < 0) {
            fprintf(stderr, "could not create directory '%s'\n", profilePath);
            exit(2);
        }
        opts->code_profile = profilePath;

        args[n++] = "-code-profile";
        args[n++] = opts->code_profile;
    }

    /* Pass boot properties to the core. First, those from boot.prop,
     * then those from the command-line */
    const FileData* bootProperties = avdInfo_getBootProperties(avd);
    if (!fileData_isEmpty(bootProperties)) {
        PropertyFileIterator iter[1];
        propertyFileIterator_init(iter,
                                  bootProperties->data,
                                  bootProperties->size);
        while (propertyFileIterator_next(iter)) {
            char temp[MAX_PROPERTY_NAME_LEN + MAX_PROPERTY_VALUE_LEN + 2];
            snprintf(temp, sizeof temp, "%s=%s", iter->name, iter->value);
            args[n++] = "-boot-property";
            args[n++] = ASTRDUP(temp);
        }
    }

    if (opts->prop != NULL) {
        ParamList*  pl = opts->prop;
        for ( ; pl != NULL; pl = pl->next ) {
            args[n++] = "-boot-property";
            args[n++] = pl->param;
        }
    }

    /* Setup the kernel init options
     */
    {
        static char  params[1024];
        char        *p = params, *end = p + sizeof(params);

        /* Don't worry about having a leading space here, this is handled
         * by the core later. */

        p = bufprint(p, end, " androidboot.hardware=goldfish");
#ifdef TARGET_I386
        p = bufprint(p, end, " clocksource=pit");
#endif

        if (opts->shell || opts->logcat) {
            p = bufprint(p, end, " androidboot.console=%s%d",
                         androidHwConfig_getKernelSerialPrefix(android_hw),
                         shell_serial );
        }

        if (!opts->no_jni) {
            p = bufprint(p, end, " android.checkjni=1");
        }

        if (opts->no_boot_anim) {
            p = bufprint( p, end, " android.bootanim=0" );
        }

        if (opts->logcat) {
            char*  q = bufprint(p, end, " androidboot.logcat=%s", opts->logcat);

            if (q < end) {
                /* replace any space by a comma ! */
                {
                    int  nn;
                    for (nn = 1; p[nn] != 0; nn++)
                        if (p[nn] == ' ' || p[nn] == '\t')
                            p[nn] = ',';
                    p += nn;
                }
            }
            p = q;
        }

        if (opts->bootchart) {
            p = bufprint(p, end, " androidboot.bootchart=%s", opts->bootchart);
        }

        if (opts->selinux) {
            p = bufprint(p, end, " androidboot.selinux=%s", opts->selinux);
        }

        if (p >= end) {
            fprintf(stderr, "### ERROR: kernel parameters too long\n");
            exit(1);
        }

        hw->kernel_parameters = strdup(params);
    }

    if (opts->ports) {
        args[n++] = "-android-ports";
        args[n++] = opts->ports;
    }

    if (opts->port) {
        args[n++] = "-android-port";
        args[n++] = opts->port;
    }

    if (opts->report_console) {
        args[n++] = "-android-report-console";
        args[n++] = opts->report_console;
    }

    if (opts->http_proxy) {
        args[n++] = "-http-proxy";
        args[n++] = opts->http_proxy;
    }

    /* Deal with camera emulation */
    if (opts->webcam_list) {
        /* List connected webcameras */
        args[n++] = "-list-webcam";
    }

    /* Set up the interfaces for inter-emulator networking */
    if (opts->shared_net_id) {
        unsigned int shared_net_id = atoi(opts->shared_net_id);
        char nic[37];

        args[n++] = "-net";
        args[n++] = "nic,vlan=0";
        args[n++] = "-net";
        args[n++] = "user,vlan=0";

        args[n++] = "-net";
        snprintf(nic, sizeof nic, "nic,vlan=1,macaddr=52:54:00:12:34:%02x", shared_net_id);
        args[n++] = strdup(nic);
        args[n++] = "-net";
        args[n++] = "socket,vlan=1,mcast=230.0.0.10:1234";
    }

#if defined(TARGET_I386) || defined(TARGET_X86_64)
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    bool accel_ok = handleCpuAcceleration(opts, avd, &accel_mode, accel_status);

    // CPU acceleration only works for x86 and x86_64 system images.
    if (accel_mode == ACCEL_OFF && accel_ok) {
        args[n++] = ASTRDUP(kDisableAccelerator);
    } else if (accel_mode == ACCEL_ON) {
        if (!accel_ok) {
            derror("CPU acceleration not supported on this machine!");
            derror("Reason: %s", accel_status);
            exit(1);
        }
        args[n++] = ASTRDUP(kEnableAccelerator);
    } else {
        args[n++] = accel_ok ? ASTRDUP(kEnableAccelerator)
                             : ASTRDUP(kDisableAccelerator);
    }

    AFREE(accel_status);
#else
    if (VERBOSE_CHECK(init)) {
        dwarning("CPU acceleration only works with x86/x86_64 "
            "system images.");
    }
#endif

    if (hw->hw_cpu_ncore > 1) {
        dwarning("Classic qemu does not support SMP. "
                "The hw.cpu.ncore option from your config file is ignored.");
    }

    while(argc-- > 0) {
        args[n++] = *argv++;
    }
    args[n] = 0;
    // We started with 128 slots in |args|.
    assert(n < 128);

    /* Generate a hardware-qemu.ini for this AVD. The real hardware
     * configuration is ususally stored in several files, e.g. the AVD's
     * config.ini plus the skin-specific hardware.ini.
     *
     * The new file will group all definitions and will be used to
     * launch the core with the -android-hw <file> option.
     */
    {
        const char* coreHwIniPath = avdInfo_getCoreHwIniPath(avd);
        CIniFile* hwIni = iniFile_newEmpty(NULL);
        androidHwConfig_write(hw, hwIni);

        if (filelock_create(coreHwIniPath) == NULL) {
            /* The AVD is already in use, we still support this as an
             * experimental feature. Use a temporary hardware-qemu.ini
             * file though to avoid overwriting the existing one. */
             TempFile*  tempIni = tempfile_create();
             coreHwIniPath = tempfile_path(tempIni);
        }

        /* While saving HW config, ignore valueless entries. This will not break
         * anything, but will significantly simplify comparing the current HW
         * config with the one that has been associated with a snapshot (in case
         * VM starts from a snapshot for this instance of emulator). */
        if (iniFile_saveToFileClean(hwIni, coreHwIniPath) < 0) {
            derror("Could not write hardware.ini to %s: %s", coreHwIniPath, strerror(errno));
            exit(2);
        }
        args[n++] = "-android-hw";
        args[n++] = strdup(coreHwIniPath);

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

    if(VERBOSE_CHECK(init)) {
        int i;
        printf("QEMU options list:\n");
        for(i = 0; i < n; i++) {
            printf("emulator: argv[%02d] = \"%s\"\n", i, args[i]);
        }
        /* Dump final command-line option to make debugging the core easier */
        printf("Concatenated QEMU options:\n");
        for (i = 0; i < n; i++) {
            /* To make it easier to copy-paste the output to a command-line,
             * quote anything that contains spaces.
             */
            if (strchr(args[i], ' ') != NULL) {
                printf(" '%s'", args[i]);
            } else {
                printf(" %s", args[i]);
            }
        }
        printf("\n");
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

    // for now there's no uses of SettingsAgent, so we don't set it
    uiEmuAgent.settings = NULL;

    /* Setup SDL UI just before calling the code */
#ifdef __linux__
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif
    if (init_sdl_ui(opts, hw, &uiEmuAgent) < 0) {
        return 1;
    }
    skin_winsys_spawn_thread(enter_qemu_main_loop, n, args);
    skin_winsys_enter_main_loop(argc, argv);

    return 0;
}
