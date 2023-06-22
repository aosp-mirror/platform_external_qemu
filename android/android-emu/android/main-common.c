/* Copyright (C) 2011-2015 The Android Open Source Project
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
#include "android/main-common.h"

#include <assert.h>                                         // for assert
#include <ctype.h>                                          // for isdigit
#include <inttypes.h>                                       // for PRIu64
#include <limits.h>                                         // for PATH_MAX

#include "android/avd/info.h"                               // for avdInfo_g...
#include "android/avd/util.h"                               // for AVD_PHONE
#include "aemu/base/gl_object_counter.h"                 // for android_g...
#include "android/camera/camera-list.h"                     // for android_c...
#include "android/cmdline-option.h"                         // for android_p...
#include "android/console.h"                                // for getConsol...
#include "host-common/crash-handler.h"              // for crashhand...
#include "android/emulation/android_pipe_unix.h"            // for android_u...
#include "android/emulation/bufprint_config_dirs.h"         // for bufprint_...
#include "android/emulation/control/globals_agent.h"        // for QAndroidG...
#include "host-common/multi_display_agent.h"  // for QAndroidM...
#include "host-common/vm_operations.h"        // for QAndroidV...
#include "host-common/feature_control.h"         // for feature_i...
#include "host-common/hw-config-helper.h"
#include "android/hw-sensors.h"
#include "android/kernel/kernel_utils.h"                    // for KERNEL_VE...
// #include "android/main-emugl.h"                             // for androidEm...
#include "android/main-help.h"                              // for emulator_...
#include "android/network/control.h"                        // for android_n...
#include "host-common/opengles.h"                               // for android_i...
#include "android/snapshot.h"                               // for snapshot_...
#include "android/telephony/phone_number.h"
#include "android/utils/bufprint.h"                         // for bufprint
#include "android/utils/debug.h"                            // for dprint
#include "android/utils/dirscanner.h"                       // for dirScanne...
#include "android/utils/dns.h"                              // for android_d...
#include "android/utils/path.h"                             // for path_exists
#include "android/utils/sockets.h"                          // for sock_addr...
#include "android/utils/stralloc.h"                         // for stralloc_...
#include "android/utils/string.h"                           // for str_reset
#include "android/utils/system.h"                           // for AFREE
#include "android/utils/x86_cpuid.h"                        // for android_g...
#include "android/version.h"                                // for EMULATOR_...

struct QAndroidEmulatorWindowAgent;
#ifdef _WIN32
#include <process.h>
#endif
#include <stdio.h>                                          // for NULL, fpr...
#include <stdlib.h>                                         // for exit, strtol
#include <string.h>                                         // for strcmp

const char* android_skin_net_speed = NULL;
const char* android_skin_net_delay = NULL;

static const int DEFAULT_PARTITION_SIZE_IN_MB = 800;

/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****            U T I L I T Y   R O U T I N E S                  *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(init)) \
            dprint(__VA_ARGS__); \
    } while (0)

// TODO(digit): Remove this!
// The plan is to move the -wipe-data and -writable-system feature to the
// top-level 'emulator' launcher program, so that the engines don't have
// to meddle with partition images, except for mounting them. The alternative
// is to add new QEMU1 and QEMU2 options to pass the corresponding flags,
// which is overkill, given this plan.
bool android_op_wipe_data = false;
bool android_op_writable_system = false;
bool emulator_has_network_option = false;

#define ONE_MB (1024 * 1024)

unsigned convertBytesToMB(uint64_t size) {
    if (size == 0)
        return 0;

    size = (size + ONE_MB - 1) >> 20;
    if (size > UINT_MAX)
        size = UINT_MAX;

    return (unsigned)size;
}

uint64_t convertMBToBytes(unsigned megaBytes) {
    return ((uint64_t)megaBytes << 20);
}

/* Return the full file path of |fileName| relative to |rootPath|.
 * Return a new heap-allocated string to be freed by the caller. Note that
 * if |fileName| is an absolute path, the function returns a copy and ignores
 * |rootPath|.
 */
static char* _getFullFilePath(const char* rootPath, const char* fileName) {
    if (path_is_absolute(fileName)) {
        return ASTRDUP(fileName);
    } else {
        char temp[PATH_MAX], *p = temp, *end = p + sizeof(temp);

        p = bufprint(temp, end, "%s" PATH_SEP "%s", rootPath, fileName);
        if (p >= end) {
            return NULL;
        }
        return ASTRDUP(temp);
    }
}

static uint64_t _adjustPartitionSize(const char* description,
                                     uint64_t imageBytes,
                                     uint64_t defaultBytes,
                                     int inAndroidBuild) {
    char temp[64];
    unsigned imageMB;
    unsigned defaultMB;

    if (imageBytes <= defaultBytes)
        return defaultBytes;

    imageMB = convertBytesToMB(imageBytes);
    defaultMB = convertBytesToMB(defaultBytes);

    if (imageMB > defaultMB) {
        snprintf(temp, sizeof temp, "(%d MB > %d MB)", imageMB, defaultMB);
    } else {
        snprintf(temp, sizeof temp, "(%" PRIu64 "  bytes > %" PRIu64 " bytes)",
                 imageBytes, defaultBytes);
    }

    if (inAndroidBuild) {
        D("%s partition size adjusted to match image file %s\n", description,
          temp);
    }

    return convertMBToBytes(imageMB);
}

/* this function is used to perform auto-detection of the
 * system directory in the case of a SDK installation.
 *
 * we want to deal with several historical usages, hence
 * the slightly complicated logic.
 *
 * NOTE: the function returns the path to the directory
 *       containing 'fileName'. this is *not* the full
 *       path to 'fileName'.
 */
static char* _getSdkImagePath(const char* fileName) {
    char temp[MAX_PATH];
    char* p = temp;
    char* end = p + sizeof(temp);
    char* q;
    char* app;

    static const char* const searchPaths[] = {
            "",                                 /* program's directory */
            "/lib/images",                      /* this is for SDK 1.0 */
            "/../platforms/android-1.1/images", /* this is for SDK 1.1 */
            NULL};

    app = bufprint_app_dir(temp, end);
    if (app >= end)
        return NULL;

    do {
        int nn;

        /* first search a few well-known paths */
        for (nn = 0; searchPaths[nn] != NULL; nn++) {
            p = bufprint(app, end, "%s", searchPaths[nn]);
            q = bufprint(p, end, "/%s", fileName);
            if (q < end && path_exists(temp)) {
                *p = 0;
                goto FOUND_IT;
            }
        }

        /* hmmm. let's assume that we are in a post-1.1 SDK
         * scan ../platforms if it exists
         */
        p = bufprint(app, end, "/../platforms");
        if (p < end) {
            DirScanner* scanner = dirScanner_new(temp);
            if (scanner != NULL) {
                int found = 0;
                const char* subdir;

                for (;;) {
                    subdir = dirScanner_next(scanner);
                    if (!subdir)
                        break;

                    q = bufprint(p, end,
                                 PATH_SEP "%s" PATH_SEP "images" PATH_SEP "%s",
                                 subdir, fileName);
                    if (q >= end || !path_exists(temp))
                        continue;

                    found = 1;
                    p = bufprint(p, end, PATH_SEP "%s" PATH_SEP "images",
                                 subdir);
                    break;
                }
                dirScanner_free(scanner);
                if (found)
                    break;
            }
        }

        /* I'm out of ideas */
        return NULL;

    } while (0);

FOUND_IT:
    // D("image auto-detection: %s/%s", temp, fileName);
    return android_strdup(temp);
}

static char* _getSdkImage(const char* path, const char* file) {
    char temp[MAX_PATH];
    char *p = temp, *end = p + sizeof(temp);

    p = bufprint(temp, end, "%s" PATH_SEP "%s", path, file);
    if (p >= end || !path_exists(temp))
        return NULL;

    return android_strdup(temp);
}

static char* _getSdkSystemImage(const char* path,
                                const char* optionName,
                                const char* file) {
    char* image = _getSdkImage(path, file);

    if (image == NULL) {
        derror("Your system directory is missing the '%s' image file.\n"
               "Please specify one with the '%s <filepath>' option",
               file, optionName);
        exit(2);
    }
    return image;
}

static void sanitizeOptions(AndroidOptions* opts) {
    /* legacy support: we used to use -system <dir> and -image <file>
     * instead of -sysdir <dir> and -system <file>, so handle this by checking
     * whether the options point to directories or files.
     */
    if (opts->image != NULL) {
        if (opts->system != NULL) {
            if (opts->sysdir != NULL) {
                derror("You can't use -sysdir, -system and -image at the same "
                       "time.\n"
                       "You should probably use '-sysdir <path> -system "
                       "<file>'.\n");
                exit(2);
            }
        }
        dwarning(
                "Please note that -image is obsolete and that -system is now "
                "used to point\n"
                "to the system image. Next time, try using '-sysdir <path> "
                "-system <file>' instead.\n");
        str_reset(&opts->sysdir, opts->system);
        str_reset(&opts->system, opts->image);
        str_reset_null(&opts->image);
    } else if (opts->system != NULL && path_is_dir(opts->system)) {
        if (opts->sysdir != NULL) {
            derror("Option -system should now be followed by a file path, not "
                   "a directory one.\n"
                   "Please use '-sysdir <path>' to point to the system "
                   "directory.\n");
            exit(1);
        }
        dwarning(
                "Please note that the -system option should now be used to "
                "point to the initial\n"
                "system image (like the obsolete -image option). To point to "
                "the system directory\n"
                "please now use '-sysdir <path>' instead.\n");

        str_reset(&opts->sysdir, opts->system);
        str_reset_null(&opts->system);
    }

    if (opts->nocache) {
        opts->no_cache = opts->nocache;
        opts->nocache = 0;
    }

    if (opts->noaudio) {
        opts->no_audio = opts->noaudio;
        opts->noaudio = 0;
    }

    if (opts->noskin) {
        opts->no_skin = opts->noskin;
        opts->noskin = 0;
    }

    /* If -no-cache is used, ignore any -cache argument */
    if (opts->no_cache) {
        str_reset_null(&opts->cache);
    }

    /* the purpose of -no-audio is to disable sound output from the emulator,
     * not to disable Audio emulation. So simply force the 'none' backends */
    if (opts->no_audio) {
        str_reset(&opts->audio, "none");
    }

    /* without a skin, the emulator cannot know what screen dimensions are
     * approriate for the virtual device, which can result in improper
     * rendering. when creating a device through the AVD manager, a "magic"
     * skin will be created that will indicate what the appropriate screen
     * dimensions are. skins like this have a name <width>x<height>, indicating
     * the appropriate framebuffer size for the device.
     */
    if (opts->no_skin) {
        dwarning(
                "the -no-skin flag is obsolete. to have a non-skinned virtual "
                "device, create one through the AVD manager");
    }

    /* we don't accept -skindir without -skin now
     * to simplify the autoconfig stuff with virtual devices
     */
    if (opts->skindir) {
        if (!opts->skin) {
            derror("the -skindir <path> option requires a -skin <name> option");
            exit(1);
        }
    }

    if (opts->bootchart) {
        char* end;
        int timeout = strtol(opts->bootchart, &end, 10);
        if (timeout == 0) {
            str_reset_null(&opts->bootchart);
        } else if (timeout < 0 || timeout > 15 * 60) {
            derror("timeout specified for -bootchart option is invalid.\n"
                   "please use integers between 1 and 900\n");
            exit(1);
        }
    }
}

static AvdInfo* createAVD(AndroidOptions* opts, int* inAndroidBuild) {
    AvdInfo* ret = NULL;
    char tmp[MAX_PATH];
    char* tmpend = tmp + sizeof(tmp);
    char* android_build_root = NULL;
    char* android_build_out = NULL;

    /* If no AVD name was given, try to find the top of the
     * Android build tree
     */
    if (opts->avd == NULL) {
        do {
            char* out = getenv("ANDROID_PRODUCT_OUT");

            if (!getConsoleAgents()->settings->android_qemu_mode())
                out = NULL;

            if (out == NULL || out[0] == 0)
                break;

            if (!path_exists(out)) {
                derror("Can't access ANDROID_PRODUCT_OUT as '%s'\n"
                       "You need to build the Android system before launching "
                       "the emulator",
                       out);
                exit(2);
            }

            android_build_root = getenv("ANDROID_BUILD_TOP");

            if (!getConsoleAgents()->settings->android_qemu_mode())
                android_build_root = NULL;

            if (android_build_root == NULL || android_build_root[0] == 0)
                break;

            if (!path_exists(android_build_root)) {
                derror("Can't find the Android build root '%s'\n"
                       "Please check the definition of the ANDROID_BUILD_TOP "
                       "variable.\n"
                       "It should point to the root of your source tree.\n",
                       android_build_root);
                exit(2);
            }
            android_build_out = out;
            D("found Android build root: %s", android_build_root);
            D("found Android build out:  %s", android_build_out);
        } while (0);
    }
    /* if no virtual device name is given, and we're not in the
     * Android build system, we'll need to perform some auto-detection
     * magic :-)
     */
    if (opts->avd == NULL && !android_build_out) {
        if (!opts->sysdir) {
            str_reset_nocopy(&opts->sysdir, _getSdkImagePath("system.img"));
            if (!opts->sysdir) {
                derror("You did not specify a virtual device name, and the "
                       "system\n"
                       "directory could not be found.\n\n"
                       "If you are an Android SDK user, please use '@<name>' "
                       "or '-avd <name>'\n"
                       "to start a given virtual device (use -list-avds to "
                       "print available ones).\n\n"

                       "Otherwise, follow the instructions in "
                       "-help-disk-images to start the emulator\n");
                exit(2);
            }
            D("autoconfig: -sysdir %s", opts->sysdir);
        }

        if (!opts->system) {
            str_reset_nocopy(
                    &opts->system,
                    _getSdkSystemImage(opts->sysdir, "-image", "system.img"));
            D("autoconfig: -system %s", opts->system);
        }

        if (!opts->vendor) {
            str_reset_nocopy(
                    &opts->vendor,
                    _getSdkSystemImage(opts->sysdir, "-vendor", "vendor.img"));
            D("autoconfig: -vendor %s", opts->vendor);
        }

        if (!opts->kernel) {
            str_reset_nocopy(
                    &opts->kernel,
                    _getSdkSystemImage(opts->sysdir, "-kernel", "kernel-qemu"));
            D("autoconfig: -kernel %s", opts->kernel);
        }

        if (!opts->ramdisk) {
            str_reset_nocopy(&opts->ramdisk,
                             _getSdkSystemImage(opts->sysdir, "-ramdisk",
                                                "ramdisk.img"));
            D("autoconfig: -ramdisk %s", opts->ramdisk);
        }

        /* if no data directory is specified, use the system directory */
        if (!opts->datadir) {
            str_reset(&opts->datadir, opts->sysdir);
            D("autoconfig: -datadir %s", opts->sysdir);
        }

        if (!opts->data) {
            /* check for userdata-qemu.img in the data directory */
            bufprint(tmp, tmpend, "%s" PATH_SEP "userdata-qemu.img",
                     opts->datadir);
            if (!path_exists(tmp)) {
                derror("You did not provide the name of an Android Virtual "
                       "Device\n"
                       "with the '-avd <name>' option. Read -help-avd for more "
                       "information.\n\n"

                       "If you *really* want to *NOT* run an AVD, consider "
                       "using '-data <file>'\n"
                       "to specify a data partition image file (I hope you "
                       "know what you're doing).\n");
                exit(2);
            }

            str_reset(&opts->data, tmp);
            D("autoconfig: -data %s", opts->data);
        }

        if (!opts->snapstorage && opts->datadir) {
            bufprint(tmp, tmpend, "%s" PATH_SEP "snapshots.img", opts->datadir);
            if (path_exists(tmp)) {
                str_reset(&opts->snapstorage, tmp);
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }
    }

    /* setup the virtual device differently depending on whether
     * we are in the Android build system or not
     */
    if (opts->avd != NULL) {
        ret = avdInfo_new(opts->avd, getConsoleAgents()->settings->avdParams(),
                          getConsoleAgents()->settings->android_cmdLineOptions()->sysdir);
        if (ret == NULL) {
            /* an error message has already been printed */
            dprint("could not find virtual device named '%s'", opts->avd);
            exit(1);
        }

        /* Did we set an ID for this AVD? */
        if (opts->id != NULL) {
            avdInfo_setAvdId(ret, opts->id);
        }
    } else {
        if (!android_build_out) {
            android_build_out = android_build_root = opts->sysdir;
        }
        ret = avdInfo_newForAndroidBuild(
                android_build_root, android_build_out,
                getConsoleAgents()->settings->avdParams(),
                getConsoleAgents()->settings->android_cmdLineOptions()->sysdir);

        if (ret == NULL) {
            D("could not start virtual device\n");
            exit(1);
        }
    }

    if (android_build_out) {
        *inAndroidBuild = 1;
    } else {
        *inAndroidBuild = 0;
    }

    return ret;
}

/*
 * handleCommonEmulatorOptions
 *
 * sets values in |hw| based on options set in |opts|
 *
 * Some values that may be set:
 *
 * kernel_path
 * hw_cpu_model
 * kernel_newDeviceNaming
 * kernel_supportsYaffs2
 * disk_ramdisk_path
 * disk_systemPartition_path
 * disk_systemPartition_initPath
 * disk_dataPartition_size
 * disk_cachePartition
 * disk_cachePartition_path
 * disk_cachePartition_size
 * hw_sdCard
 * hw_sdCard_path
 * hw_ramSize
 */
static bool emulator_handleCommonEmulatorOptions(AndroidOptions* opts,
                                                 AndroidHwConfig* hw,
                                                 AvdInfo* avd,
                                                 bool is_qemu2) {
    int forceArmv7 = 0;

    // Kernel options
    {
        char* kernelFile = opts->kernel;
        int kernelFileLen;

        if (kernelFile == NULL) {
            kernelFile = is_qemu2 ? avdInfo_getRanchuKernelPath(avd)
                                  : avdInfo_getKernelPath(avd);
            if (kernelFile == NULL) {
                derror("This AVD's configuration is missing a kernel file! "
                       "Please ensure the file \"%s\" is in the same location "
                       "as your system image.",
                       is_qemu2 ? "kernel-ranchu" : "kernel-qemu");
                const char* sdkRootDir = getenv("ANDROID_SDK_ROOT");
                if (sdkRootDir) {
                    derror("ANDROID_SDK_ROOT is defined (%s) but cannot find "
                           "kernel file in "
                           "%s" PATH_SEP "system-images" PATH_SEP
                           " sub directories",
                           sdkRootDir, sdkRootDir);
                } else {
                    derror("ANDROID_SDK_ROOT is undefined");
                }
                return false;
            }
            D("autoconfig: -kernel %s", kernelFile);
        }
        if (!path_exists(kernelFile)) {
            derror("Invalid or missing kernel image file: %s", kernelFile);
            free(kernelFile);
            return false;
        }

        str_reset_nocopy(&hw->kernel_path, kernelFile);

        /* If the kernel image name ends in "-armv7", then change the cpu
         * type automatically. This is a poor man's approach to configuration
         * management, but should allow us to get past building ARMv7
         * system images with dex preopt pass without introducing too many
         * changes to the emulator sources.
         *
         * XXX:
         * A 'proper' change would require adding some sort of hardware-property
         * to each AVD config file, then automatically determine its value for
         * full Android builds (depending on some environment variable), plus
         * some build system changes. I prefer not to do that for now for
         * reasons of simplicity.
         */
        kernelFileLen = strlen(kernelFile);
        if (kernelFileLen > 6 &&
            !memcmp(kernelFile + kernelFileLen - 6, "-armv7", 6)) {
            forceArmv7 = 1;
        }
    }

    /* If the target ABI is armeabi-v7a, we can auto-detect the cpu model
     * as a cortex-a8, instead of the default (arm926) which only emulates
     * an ARMv5TE CPU.
     */
    if (!forceArmv7 && hw->hw_cpu_model[0] == '\0') {
        char* abi = avdInfo_getTargetAbi(avd);
        if (abi != NULL) {
            if (!strcmp(abi, "armeabi-v7a")) {
                forceArmv7 = 1;
            }
            AFREE(abi);
        }
    }

    /* If the target architecture is 'x86', ensure that the 'qemu32'
     * CPU model is used. Otherwise, the default (which is now 'qemu64')
     * will result in a failure to boot with some kernels under
     * un-accelerated emulation.
     */
    if (hw->hw_cpu_model[0] == '\0') {
        char* arch = avdInfo_getTargetCpuArch(avd);
        D("Target arch = '%s'", arch ? arch : "NULL");
        if (arch != NULL && !strcmp(arch, "x86")) {
            str_reset(&hw->hw_cpu_model, "qemu32");
            D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
        }
        AFREE(arch);
    }

    if (forceArmv7 != 0) {
        str_reset(&hw->hw_cpu_model, "cortex-a8");
        D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
    }

    if (hw->hw_cpu_model[0] == '\0') {
        char* abi = avdInfo_getTargetAbi(avd);
        if (abi && !strcmp(abi, "mips32r6")) {
            str_reset(&hw->hw_cpu_model, "mips32r6-generic");
            D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
        } else if (abi && !strcmp(abi, "mips32r5")) {
            str_reset(&hw->hw_cpu_model, "P5600");
            D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
        }
        AFREE(abi);
    }

    KernelVersion kernelVersion = KERNEL_VERSION_0;
    if (!android_getKernelVersion(hw->kernel_path, &kernelVersion)) {
        derror("Can't get kernel version from the kernel image file: '%s'",
               hw->kernel_path);
        return false;
    }

    // make sure we're using the proper engine (qemu1/qemu2) for the kernel
    if (is_qemu2 && kernelVersion < KERNEL_VERSION_3_10_0) {
        derror("New emulator backend requires minimum kernel version 3.10+ "
               "(currently got lower)\n"
               "Please make sure you've got updated system images and do not "
               "force the specific "
               "kernel image together with the engine version");
        return false;
    } else if (!is_qemu2 && kernelVersion >= KERNEL_VERSION_3_10_0) {
        char* kernel_file = path_basename(hw->kernel_path);
        if (kernel_file && !strcmp(kernel_file, "kernel-ranchu")) {
            derror("This kernel requires the new emulation engine\n"
                   "Please do not force the specific kernel image together "
                   "with the engine version");
            return false;
        }
        free(kernel_file);
    }

    // Auto-detect kernel device naming scheme if needed.
    if (androidHwConfig_getKernelDeviceNaming(hw) < 0) {
        const char* newDeviceNaming = "no";
        if (kernelVersion >= KERNEL_VERSION_3_10_0) {
            D("Auto-detect: Kernel image requires new device naming scheme.");
            newDeviceNaming = "yes";
        } else {
            D("Auto-detect: Kernel image requires legacy device naming "
              "scheme.");
        }
        str_reset(&hw->kernel_newDeviceNaming, newDeviceNaming);
    }

    // Auto-detect YAFFS2 partition support if needed.
    if (androidHwConfig_getKernelYaffs2Support(hw) < 0) {
        // Essentially, anything before API level 20 supports Yaffs2
        const char* newYaffs2Support = "no";
        if (avdInfo_getApiLevel(avd) < 20) {
            newYaffs2Support = "yes";
            D("Auto-detect: Kernel does support YAFFS2 partitions.");
        } else {
            D("Auto-detect: Kernel does not support YAFFS2 partitions.");
        }
        str_reset(&hw->kernel_supportsYaffs2, newYaffs2Support);
    }

    /* opts->ramdisk is never NULL (see createAVD) here */
    if (opts->ramdisk) {
        str_reset(&hw->disk_ramdisk_path, opts->ramdisk);
    } else if (!hw->disk_ramdisk_path[0]) {
        char* ramdiskPath = avdInfo_getRamdiskPath(avd);
        str_reset_nocopy(&hw->disk_ramdisk_path, ramdiskPath);
        D("autoconfig: -ramdisk %s", hw->disk_ramdisk_path);
    }

    if (opts->logcat_output) {
        str_reset(&getConsoleAgents()->settings->hw()->hw_logcatOutput_path, opts->logcat_output);
    }

    if (opts->quit_after_boot) {
        char* end;
        hw->test_quitAfterBootTimeOut = strtol(opts->quit_after_boot, &end, 0);
        if (hw->test_quitAfterBootTimeOut <= 0) {
            derror("-quit-after-boot must be followed by a positive timeout in "
                   "seconds");
            exit(1);
        } else {
            D("Will quit emulator after guest boots completely, or after %d "
              "seconds",
              hw->test_quitAfterBootTimeOut);
        }
    }

    if (opts->delay_adb) {
        hw->test_delayAdbTillBootComplete = 1;
    }

    if (opts->monitor_adb) {
        char* end;
        hw->test_monitorAdb = strtol(opts->monitor_adb, &end, 0);
        if (hw->test_monitorAdb <= 0) {
            derror("-monitor-adb must be followed by a positive number");
            exit(1);
        } else {
            D("Will monitor adb messages to the guest");
        }
    }

    /* -partition-size is used to specify the max size of both the system
     * and data partition sizes.
     */
    uint64_t defaultPartitionSize =
            convertMBToBytes(DEFAULT_PARTITION_SIZE_IN_MB);

    if (opts->partition_size) {
        char* end;
        long sizeMB = strtol(opts->partition_size, &end, 0);
        long minSizeMB = 10;
        long maxSizeMB = LONG_MAX / ONE_MB;

        if (sizeMB < 0 || *end != 0) {
            derror("-partition-size must be followed by a positive integer");
            exit(1);
        }
        if (sizeMB < minSizeMB || sizeMB > maxSizeMB) {
            derror("partition-size (%d) must be between %dMB and %dMB", sizeMB,
                   minSizeMB, maxSizeMB);
            exit(1);
        }
        defaultPartitionSize = (uint64_t)sizeMB * ONE_MB;
    }

    /** SYSTEM PARTITION **/

    if (opts->sysdir == NULL) {
        if (avdInfo_inAndroidBuild(avd)) {
            str_reset(&opts->sysdir, avdInfo_getContentPath(avd));
            D("autoconfig: -sysdir %s", opts->sysdir);
        }
    }

    if (opts->sysdir != NULL) {
        if (!path_exists(opts->sysdir)) {
            derror("Directory does not exist: %s", opts->sysdir);
            return false;
        }
    }

    {
        char* rwImage = NULL;
        char* initImage = NULL;

        do {
            if (opts->system == NULL) {
                /* If -system is not used, try to find a runtime system image
                 * (i.e. system-qemu.img) in the content directory.
                 */
                rwImage = avdInfo_getSystemImagePath(avd);
                if (rwImage != NULL) {
                    break;
                }
                /* Otherwise, try to find the initial system image */
                initImage = avdInfo_getSystemInitImagePath(avd);
                if (initImage == NULL) {
                    derror("No initial system image for this configuration!");
                    exit(1);
                }
                break;
            }

            /* If -system <name> is used, use it to find the initial image */
            if (opts->sysdir != NULL && !path_exists(opts->system)) {
                initImage = _getFullFilePath(opts->sysdir, opts->system);
            } else {
                initImage = ASTRDUP(opts->system);
            }
            if (!path_exists(initImage)) {
                derror("System image file doesn't exist: %s", initImage);
                exit(1);
            }

        } while (0);

        if (rwImage != NULL) {
            /* Use the read/write image file directly */
            str_reset_nocopy(&hw->disk_systemPartition_path, rwImage);
            str_reset_null(&hw->disk_systemPartition_initPath);
            D("Using direct system image: %s", rwImage);
        } else if (initImage != NULL) {
            str_reset_null(&hw->disk_systemPartition_path);
            str_reset_nocopy(&hw->disk_systemPartition_initPath, initImage);
            D("Using initial system image: %s", initImage);
        }

        /* Check the size of the system partition image.
         * If we have an AVD, it must be smaller than
         * the disk.systemPartition.size hardware property.
         *
         * Otherwise, we need to adjust the systemPartitionSize
         * automatically, and print a warning.
         *
         */
        const char* systemImage = hw->disk_systemPartition_path;
        uint64_t systemBytes;

        if (systemImage == NULL)
            systemImage = hw->disk_systemPartition_initPath;

        if (path_get_size(systemImage, &systemBytes) < 0) {
            derror("Missing system image: %s", systemImage);
            return false;
        }

        hw->disk_systemPartition_size = _adjustPartitionSize(
                "system", systemBytes, defaultPartitionSize,
                avdInfo_inAndroidBuild(avd));
    }

    /** VENDOR PARTITION **/
    {
        char* rwImage = NULL;
        char* initImage = NULL;

        do {
            if (opts->vendor == NULL) {
                /* If -vendor is not used, try to find a runtime system image
                 * (i.e. vendor-qemu.img) in the content directory.
                 */
                rwImage = avdInfo_getVendorImagePath(avd);
                if (rwImage != NULL) {
                    break;
                }
                /* Otherwise, try to find the initial system image */
                initImage = avdInfo_getVendorInitImagePath(avd);
                /* Vendor image is for O+ only */
                break;
            }

            /* If -vendor <name> is used, use it to find the initial image */
            if (opts->sysdir != NULL && !path_exists(opts->vendor)) {
                initImage = _getFullFilePath(opts->sysdir, opts->vendor);
            } else {
                initImage = ASTRDUP(opts->vendor);
            }
            /* Vendor image is for O+ only */
            if (!path_exists(initImage)) {
                initImage = NULL;
            }

        } while (0);

        if (rwImage != NULL) {
            /* Use the read/write image file directly */
            str_reset_nocopy(&hw->disk_vendorPartition_path, rwImage);
            str_reset_null(&hw->disk_vendorPartition_initPath);
            D("Using direct vendor image: %s", rwImage);
        } else if (initImage != NULL) {
            str_reset_null(&hw->disk_vendorPartition_path);
            str_reset_nocopy(&hw->disk_vendorPartition_initPath, initImage);
            D("Using initial vendor image: %s", initImage);
        } else {
            str_reset_null(&hw->disk_vendorPartition_path);
            str_reset_null(&hw->disk_vendorPartition_initPath);
            D("No vendor image");
        }

        /* Check the size of the vendor partition image.
         * If we have an AVD, it must be smaller than
         * the disk.systemPartition.size hardware property.
         *
         * Otherwise, we need to adjust the systemPartitionSize
         * automatically, and print a warning.
         *
         */
        const char* vendorImage = hw->disk_vendorPartition_path;
        uint64_t vendorBytes = 0;

        if (vendorImage == NULL) {
            vendorImage = hw->disk_vendorPartition_initPath;
        }

        if (vendorImage) {
            if (path_get_size(vendorImage, &vendorBytes) < 0) {
                derror("Missing vendor image: %s", vendorImage);
                return false;
            }

            hw->disk_vendorPartition_size = _adjustPartitionSize(
                    "vendor", vendorBytes, defaultPartitionSize,
                    avdInfo_inAndroidBuild(avd));
        }
    }

    /** DATA PARTITION **/

    if (opts->datadir) {
        if (!path_exists(opts->datadir)) {
            derror("Invalid -datadir directory: %s", opts->datadir);
        }
    }

    {
        char* dataImage = NULL;
        char* initImage = NULL;

        do {
            if (!opts->data) {
                dataImage = avdInfo_getDataImagePath(avd);
                if (dataImage != NULL) {
                    D("autoconfig: -data %s", dataImage);
                    break;
                }
                dataImage = avdInfo_getDefaultDataImagePath(avd);
                if (dataImage == NULL) {
                    derror("No data image path for this configuration!");
                    return false;
                }
                opts->wipe_data = 1;
                break;
            }

            if (opts->datadir) {
                dataImage = _getFullFilePath(opts->datadir, opts->data);
            } else {
                dataImage = ASTRDUP(opts->data);
            }
        } while (0);

        if (opts->initdata != NULL) {
            initImage = ASTRDUP(opts->initdata);
            if (!path_exists(initImage)) {
                derror("Invalid initial data image path: %s", initImage);
                return false;
            }
        } else {
            initImage = avdInfo_getDataInitImagePath(avd);
            D("autoconfig: -initdata %s", initImage);
        }

        str_reset(&hw->disk_dataPartition_path, dataImage);
        if (opts->wipe_data) {
            str_reset(&hw->disk_dataPartition_initPath, initImage);
            free(initImage);
        } else {
            str_reset_null(&hw->disk_dataPartition_initPath);
        }
        // ignore wipedata
        if (!feature_is_enabled(kFeature_DownloadableSnapshot)) {
            android_op_wipe_data = opts->wipe_data;
        }
        android_op_writable_system = opts->writable_system;

        uint64_t defaultBytes = hw->disk_dataPartition_size;
        if (defaultBytes == 0 || opts->partition_size) {
            // If the -partition-size option is given that should override
            // whatever setting was in the config file.
            defaultBytes = defaultPartitionSize;
            if (opts->partition_size) {
                hw->disk_dataPartition_size =
                        atoi(opts->partition_size) * 1048576ULL;
            } else {
                hw->disk_dataPartition_size = defaultBytes;
            }
        } else {
            hw->disk_dataPartition_size = defaultBytes;
        }
    }

    /** CACHE PARTITION **/

    if (opts->no_cache) {
        /* No cache partition at all */
        hw->disk_cachePartition = 0;
    } else if (!hw->disk_cachePartition) {
        if (opts->cache) {
            dwarning(
                    "Emulated hardware doesn't support a cache partition. "
                    "-cache option ignored!");
            str_reset_null(&opts->cache);
        }
    } else {
        if (!opts->cache) {
            /* Find the current cache partition file */
            str_reset_nocopy(&opts->cache, avdInfo_getCachePath(avd));
            if (!opts->cache) {
                str_reset_nocopy(&opts->cache,
                                 avdInfo_getDefaultCachePath(avd));
            }
            if (opts->cache) {
                D("autoconfig: -cache %s", opts->cache);
            }
        }

        if (opts->cache) {
            str_reset(&hw->disk_cachePartition_path, opts->cache);
        }
    }

    if (hw->disk_cachePartition_path && opts->cache_size) {
        /* Set cache partition size per user options. */
        char* end;
        long sizeMB = strtol(opts->cache_size, &end, 0);

        if (sizeMB < 0 || *end != 0) {
            derror("-cache-size must be followed by a positive integer");
            return false;
        }
        hw->disk_cachePartition_size = (uint64_t)sizeMB * ONE_MB;
    }

    /** ENCRYPTION KEY PARTITION */
    if (opts->encryption_key) {
        str_reset(&hw->disk_encryptionKeyPartition_path, opts->encryption_key);
    } else {
        str_reset_nocopy(&hw->disk_encryptionKeyPartition_path,
                         avdInfo_getEncryptionKeyImagePath(avd));
    }

    /** SD CARD PARTITION */

    if (feature_is_enabled(kFeature_DownloadableSnapshot)) {
        if (hw->hw_sdCard) {
            hw->hw_sdCard = false;
            dinfo("Disabling SD Card suppor due to DownloadableSnapshot "
                  "feature");
        }
    }

    if (!hw->hw_sdCard) {
        /* No SD Card emulation, so -sdcard will be ignored */
        if (opts->sdcard) {
            dwarning(
                    "Emulated hardware doesn't support SD Cards. -sdcard "
                    "option ignored.");
            str_reset_null(&opts->sdcard);
        }
    } else {
        /* Auto-configure -sdcard if it is not available */
        if (!opts->sdcard) {
            do {
                /* If -datadir <path> is used, look for a sdcard.img file here
                 */
                if (opts->datadir) {
                    char tmp[PATH_MAX], *tmpend = tmp + sizeof(tmp);
                    bufprint(tmp, tmpend, "%s" PATH_SEP "%s", opts->datadir,
                             "system.img");
                    if (path_exists(tmp)) {
                        str_reset(&opts->sdcard, tmp);
                        break;
                    }
                }

                /* Otherwise, look at the AVD's content */
                str_reset_nocopy(&opts->sdcard, avdInfo_getSdCardPath(avd));
                if (opts->sdcard != NULL) {
                    break;
                }

                /* Nothing */
            } while (0);

            if (opts->sdcard) {
                D("autoconfig: -sdcard %s", opts->sdcard);
            }
        }
    }

    if (opts->sdcard) {
        uint64_t size;
        if (path_get_size(opts->sdcard, &size) == 0) {
            /* see if we have an sdcard image.  get its size if it exists */
            /* due to what looks like limitations of the MMC protocol, one has
             * to use an SD Card image that is equal or larger than 9 MB
             */
            if (size < 9 * 1024 * 1024ULL &&
                !feature_is_enabled(kFeature_DownloadableSnapshot)) {
                dwarning("SD Card files must be at least 9MB, ignoring '%s'",
                         opts->sdcard);
            } else {
                str_reset(&hw->hw_sdCard_path, opts->sdcard);
                avdInfo_setImageFile(avd, AVD_IMAGE_SDCARD, opts->sdcard);
            }
        } else {
            dwarning("no SD Card image at '%s'", opts->sdcard);
        }
    }

    if (opts->selinux) {
        if ((strcmp(opts->selinux, "permissive") != 0) &&
            (strcmp(opts->selinux, "disabled") != 0)) {
            derror("-selinux must be \"disabled\" or \"permissive\"");
            return false;
        }

        // SELinux 'disabled' mode is no longer supported starting with M.
        // See https://android-review.googlesource.com/#/c/148538/
        const int kSELinuxWithoutDisabledApiLevel = 23;
        if (!strcmp(opts->selinux, "disabled") &&
            avdInfo_getApiLevel(avd) >= kSELinuxWithoutDisabledApiLevel) {
            dwarning(
                    "SELinux 'disabled' is no longer supported starting "
                    "with API level %d, switching to 'permissive'",
                    kSELinuxWithoutDisabledApiLevel);
            str_reset(&opts->selinux, "permissive");
        }
    }

    if (opts->memory) {
        // override avd memory setting
        char* end;
        long ramSize = strtol(opts->memory, &end, 0);
        if (ramSize < 0 || *end != 0) {
            derror("-memory must be followed by a positive integer");
            return false;
        }
        hw->hw_ramSize = ramSize;
    }

    if (hw->hw_ramSize <= 0) {
        /* Compute the default RAM size based on the size of screen.
         * This is only used when the skin doesn't provide the ram
         * size through its hardware.ini (i.e. legacy ones) or when
         * in the full Android build system.
         */
        int64_t pixels = hw->hw_lcd_width * hw->hw_lcd_height;
        /* The following thresholds are a bit liberal, but we
         * essentially want to ensure the following mappings:
         *
         *   320x480 -> 96
         *   800x600 -> 128
         *  1024x768 -> 256
         *
         * These are just simple heuristics, they could change in
         * the future.
         */
        if (pixels <= 250000)
            hw->hw_ramSize = 96;
        else if (pixels <= 500000)
            hw->hw_ramSize = 128;
        else
            hw->hw_ramSize = 256;
    }

    // all 64 bit archs we support include "64"
    bool guest_is_32_bit = strstr(hw->hw_cpu_arch, "64") == 0;
    bool host_is_32_bit = sizeof(void*) == 4;
    bool limit_is_4gb = (guest_is_32_bit || host_is_32_bit);

    // bug: 277897783
    if (android_foldable_hinge_configured()) {
        int height_in_config = hw->hw_displayRegion_0_1_height;
        if (height_in_config > hw->hw_lcd_height) {
            hw->hw_displayRegion_0_1_height = hw->hw_lcd_height;
            dwarning(
                    "folded height %d is larger than lcd height %d, reduced to "
                    "lcd height.",
                    height_in_config, hw->hw_lcd_height);
        }
    }

    if (avdInfo_getApiLevel(avd) >= 33) {
        hw->userdata_useQcow2 = true;
    }

    // enforce CDD minimums
    int minRam = 32;
    bool isFoldable = android_foldable_any_folded_area_configured() ||
                      android_foldable_hinge_configured() ||
                      android_foldable_rollable_configured();
    bool isLargeScreen =
            ((long long)(hw->hw_lcd_width) * (long long)(hw->hw_lcd_height)) >=
            3LL * 1000LL * 1000LL;

    if (avdInfo_getApiLevel(avd) >= 34) {
        minRam = 2560;  // 2.5G is required for U and up, to avoid kswapd eating
                        // cpus
    }
    if (avdInfo_getApiLevel(avd) >= 33 && (isFoldable || isLargeScreen)) {
        minRam = 3072; // 3G is required for U and up, to avoid kswapd eating cpus
        D("foldable or large screen devices with api >=33 is set to have "
          "minimum ram 3G");
    } else if (avdInfo_getApiLevel(avd) >= 29) {
        // bug: 129958266
        // Q preview where API level is not 29 yet,
        // it becomes 1000 so we still get the effect we want.
        minRam = 2048;
        // To avoid lmk, 4096 mb is required. but that makes it a
        // non-starter for running on most user machines :/
    } else if (avdInfo_getApiLevel(avd) >= 26) {
        // bug: 112664026
        // This isn't a CDD minimum, but could be a source of why
        // people think the emulator is slow recently.
        minRam = 1536;
    } else if (avdInfo_getApiLevel(avd) >= 21) {
        if (guest_is_32_bit) {
            // TODO: adjust min based on screen size, wear, 23+
            // android wear min is actually 416 but most people boot phones
            minRam = 512;
        } else {
            minRam = 832;
        }
        if (!host_is_32_bit) {
            // This isn't a CDD minimum but was present in earlier versions of
            // the emulator For recent system versions, ensure a minimum of 1GB
            // or memory, anything lower is very painful during the boot process
            // and after that.
            minRam = 1024;
        }
    } else if (avdInfo_getApiLevel(avd) >= 14) {
        minRam = 340;
    } else if (avdInfo_getApiLevel(avd) >= 9) {
        minRam = 128;
    } else if (avdInfo_getApiLevel(avd) >= 7) {
        minRam = 92;
    }

    /* just remove any lower bound for low ram device:
       Trust users know what they are doing with explicit -lowram option */
    if (opts->lowram) {
        D("Removing any lower bound of RAM size");
        minRam = 0;
    }

    if (hw->hw_ramSize < minRam) {
        dinfo("Increasing RAM size to %iMB", minRam);
        hw->hw_ramSize = minRam;
    } else if (limit_is_4gb && hw->hw_ramSize > 4096) {
        D("Decreasing RAM size to 4096MB");
        hw->hw_ramSize = 4096;
    } else {
        D("Physical RAM size: %dMB", hw->hw_ramSize);

        // 101135008
        // 101268816
    }

    // Set the VM heap size to min vm heap per api level, but make sure we don't
    // overconstrain it - so increase it to RAM size / 4 up to 576 MB.
    // Larger VM heaps usually don't add any good, but only slow down the GC.
    // Also, we won't be able to allocate too huge single chunk or memory for a
    // heap anyway.
    const int minApiLevelVmHeapSize =
            androidHwConfig_getMinVmHeapSize(hw, avdInfo_getApiLevel(avd));
    const int minRamVmHeapSize = hw->hw_ramSize / 4;
    int minVmHeapSize = minRamVmHeapSize > minApiLevelVmHeapSize
                                ? minRamVmHeapSize
                                : minApiLevelVmHeapSize;
    const int maxVmHeapSize = 4 * minVmHeapSize > 576 ? 576 : 4 * minVmHeapSize;

    if (minVmHeapSize > maxVmHeapSize) {
        minVmHeapSize = maxVmHeapSize;
    }

    if (hw->vm_heapSize < minVmHeapSize) {
        D("VM heap size %iMB is below hardware specified minimum of %iMB,"
          "setting it to that value",
          hw->vm_heapSize, minVmHeapSize);

        hw->vm_heapSize = minVmHeapSize;

        const int minRamSize = minVmHeapSize * 2;
        if (hw->hw_ramSize < minRamSize) {
            hw->hw_ramSize = minRamSize;
            D("Increasing RAM to %iMB to accomodate min VM heap", minRamSize);
        }
    }

    if (hw->vm_heapSize > maxVmHeapSize) {
        D("VM heap size %iMB is above maximum supported %iMB, "
          "setting it to that value",
          hw->vm_heapSize, maxVmHeapSize);
        hw->vm_heapSize = maxVmHeapSize;
    }

    const bool is_qemu1 = !is_qemu2;
    if (is_qemu1 && avdInfo_getSnapshotPresent(avd)) {
        const bool load_previous_snapshot = !opts->no_snapshot_load;
        const bool save_snapshot_on_exit = !opts->no_snapshot_save;
        if (load_previous_snapshot || save_snapshot_on_exit) {
            android_op_writable_system = true;
        }
    }

    if (!android_op_writable_system) {
        D("System image is read only");
    } else {
        dwarning("System image is writable");
    }

    return true;
}

const char* getAcceleratorEnableParam(AndroidCpuAccelerator accel_type) {
    switch (accel_type) {
        case ANDROID_CPU_ACCELERATOR_KVM:
            return "-enable-kvm";
        case ANDROID_CPU_ACCELERATOR_HAX:
            return "-enable-hax";
        case ANDROID_CPU_ACCELERATOR_HVF:
            return "-enable-hvf";
        case ANDROID_CPU_ACCELERATOR_WHPX:
            return "-enable-whpx";
        case ANDROID_CPU_ACCELERATOR_AEHD:
            return "-enable-aehd";
        default:
            return "";
    }
}

bool handleCpuAcceleration(AndroidOptions* opts,
                           const AvdInfo* avd,
                           CpuAccelMode* accel_mode,
                           char** accel_status) {
    assert(accel_status != NULL);

    /* Handle CPU acceleration options. */
    if (opts->no_accel) {
        if (opts->accel) {
            if (strcmp(opts->accel, "off") != 0) {
                derror("You cannot use -no-accel and '-accel %s' at the same "
                       "time",
                       opts->accel);
                exit(1);
            }
        } else {
            str_reset(&opts->accel, "off");
        }
    }

    *accel_mode = ACCEL_AUTO;
    if (opts->accel) {
        if (!strcmp(opts->accel, "off")) {
            *accel_mode = ACCEL_OFF;
        } else if (!strcmp(opts->accel, "on")) {
            *accel_mode = ACCEL_ON;
        } else if (!strcmp(opts->accel, "auto")) {
            *accel_mode = ACCEL_AUTO;
        } else {
            derror("Invalid '-accel %s' parameter, valid values are: on off "
                   "auto\n",
                   opts->accel);
            exit(1);
        }
    }

    AndroidCpuAcceleration accel_capability =
            androidCpuAcceleration_getStatus(accel_status);
    bool accel_ok = (accel_capability == ANDROID_CPU_ACCELERATION_READY);
    // Dump CPU acceleration status.
    if (VERBOSE_CHECK(init)) {
        const char* accel_str = "DISABLED";
        if (accel_ok) {
            if (*accel_mode == ACCEL_OFF) {
                accel_str = "working, but disabled by user";
            } else {
                accel_str = "working";
            }
        }
        dprint("CPU Acceleration: %s", accel_str);
        dprint("CPU Acceleration status: %s", *accel_status);
    }

    // Special case: x86/x86_64 emulation currently requires hardware
    // acceleration, so refuse to start in 'auto' mode if it is not
    // available.
    {
        char* abi = avdInfo_getTargetAbi(avd);
#if defined(__APPLE__) && defined(__arm64__)
        if (!strncmp(abi, "arm64", 3)) {
#else
        if (!strncmp(abi, "x86", 3)) {
#endif
            dprint("%s: feature check for hvf", __func__);
            // select HVF on Mac if available and we are running on x86
            // TODO: Fix x86_64 support in HVF
            const bool hvf_is_ok =
                    feature_is_enabled(kFeature_HVF) &&
                    androidCpuAcceleration_isAcceleratorSupported(
                            ANDROID_CPU_ACCELERATOR_HVF);

            if (getConsoleAgents()->settings->android_qemu_mode() && !hvf_is_ok && !accel_ok &&
                *accel_mode != ACCEL_OFF) {
                derror("%s emulation currently requires hardware "
                       "acceleration!\n"
                       "CPU acceleration status: %s\n"
                       "More info on configuring VM acceleration on "
#ifdef _WIN32
                       "Windows:\n"
                       "https://developer.android.com/studio/run/"
                       "emulator-acceleration#vm-windows\n"
#else  // _WIN32
#ifdef __APPLE__
                        "macOS:\n"
                        "https://developer.android.com/studio/run/"
                        "emulator-acceleration#vm-mac\n"
#else   // __APPLE__
                        "Linux:\n"
                        "https://developer.android.com/studio/run/"
                        "emulator-acceleration#vm-linux\n"
#endif  // !__APPLE__
#endif  // !_WIN32
                       "General information on acceleration: "
                       "https://developer.android.com/studio/run/"
                       "emulator-acceleration.\n",
                       abi, *accel_status);
                exit(1);
            } else if (*accel_mode == ACCEL_OFF ||
                       (*accel_mode == ACCEL_OFF && !getConsoleAgents()->settings->android_qemu_mode())) {
                if (!getConsoleAgents()->settings->android_qemu_mode()) {
                    *accel_mode = ACCEL_OFF;
                }
                // '-no-accel' of '-accel off' was used explicitly. Warn about
                // the issue but do not exit.
                dwarning(
                        "%s emulation may not work without hardware "
                        "acceleration!",
                        abi);
            } else {
#ifndef __arm64__
                /* CPU acceleration is enabled and working, but if the host CPU
                 * does not support all instruction sets specified in the x86/
                 * x86_64 ABI, emulation may fail on unsupported instructions.
                 * Therefore, check the capabilities of the host CPU and warn
                 * the user if any required features are missing. */
                uint32_t ecx = 0;
                char buf[64], *p = buf, *const end = p + sizeof(buf);

                /* Execute CPUID instruction with EAX=1 and ECX=0 to get CPU
                 * feature bits (stored in EDX, ECX and EBX). */
                android_get_x86_cpuid(1, 0, NULL, NULL, &ecx, NULL);

                /* Theoretically, MMX and SSE/2/3 should be checked as well, but
                 * CPU models that do not support them are probably too old to
                 * run Android emulator. */
                if (!(ecx & CPUID_ECX_SSSE3)) {
                    p = bufprint(p, end, " SSSE3");
                }
                if (!strcmp(abi, "x86_64")) {
                    if (!(ecx & CPUID_ECX_SSE41)) {
                        p = bufprint(p, end, " SSE4.1");
                    }
                    if (!(ecx & CPUID_ECX_SSE42)) {
                        p = bufprint(p, end, " SSE4.2");
                    }
                    if (!(ecx & CPUID_ECX_POPCNT)) {
                        p = bufprint(p, end, " POPCNT");
                    }
                }

                if (p > buf) {
                    /* Using dwarning(..) would cause this message to be written
                     * to stdout and filtered out by AVD Manager. But we want
                     * the AVD Manager user to see this warning, so we resort to
                     * fprintf(..). */
                    dwarning(
                            "Host CPU is missing the"
                            " following feature(s) required for %s "
                            "emulation:%s",
                            abi, buf);
                    dwarning(
                            "Hardware-accelerated emulation may not work "
                            "properly!");
                }
#endif

                if (hvf_is_ok) {
                    androidCpuAcceleration_resetCpuAccelerator(
                            ANDROID_CPU_ACCELERATOR_HVF);
                    *accel_mode = ACCEL_HVF;
                }
            }
        }

        AFREE(abi);
    }

    if (*accel_mode == ACCEL_OFF) {
        androidCpuAcceleration_resetCpuAccelerator(
                ANDROID_CPU_ACCELERATOR_NONE);
    }

    return accel_ok;
}

// _findQemuInformationalOption: search for informational QEMU options
//
// Scans the given command-line options for any informational QEMU option (see
// |qemu_info_opts| for the list of informational QEMU options). Returns the
// first matching option, or NULL if no match is found.
//
// |qemu_argc| is the number of command-line options in |qemu_argv|.
// |qemu_argv| is the array of command-line options to be searched. It is the
// caller's responsibility to ensure that all these options are intended for
// QEMU.
static char* _findQemuInformationalOption(int qemu_argc, char** qemu_argv) {
    /* Informational QEMU options, which make QEMU print some information to
     * the console and exit. */
    static const char* const qemu_info_opts[] = {
            "-h",          "-help", "-version",
            "-audio-help", "?", /* e.g. '-cpu ?' for listing available CPU
                                   models */
            "help", /* e.g. '-cpu help, -machine help', '-soundhw help' */
            NULL    /* denotes the end of the list */
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

bool emulator_parseCommonCommandLineOptions(int* p_argc,
                                            char*** p_argv,
                                            const char* targetArch,
                                            bool is_qemu2,
                                            AndroidOptions* opts,
                                            AndroidHwConfig* hw,
                                            AvdInfo** the_avd,
                                            int* exit_status) {
    *exit_status = 1;

    if (android_parse_options(p_argc, p_argv, opts) < 0) {
        return false;
    }

    LoggingFlags log_opts = kLogDefaultOptions;

    if (!opts->log_nofilter) {
        log_opts |= kLogEnableDuplicateFilter;
        dinfo("Duplicate loglines will be removed, if you wish to see each "
              "individual line launch with the -log-nofilter flag.");
    }

    if (opts->log_detailed || VERBOSE_CHECK(log)) {
        log_opts |= kLogEnableVerbose;
    }

    if (VERBOSE_CHECK(time)) {
        log_opts |= kLogEnableTime;
    }

    base_configure_logs(log_opts);
    getConsoleAgents()->settings->inject_cmdLineOptions(opts);

    // BUG: 143949261
    //
    // We are deprecating the no_window boolean flag.  no_window is now an
    // inherent property of the binary (qemu-system-***-headless)
    //
    // TODO: After this technique proves stable, go through the code and remove
    // all paths that depend on no_window, then add back a different variable
    // to track it based on the build being qemu-system-***-headless or not
    opts->no_window = false;

    if (opts->fuchsia) {
        getConsoleAgents()->settings->set_android_qemu_mode(false);
        *exit_status = EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER;
        return false;
    }

    opts->ranchu = is_qemu2;

    while ((*p_argc)-- > 1) {
        const char* opt = (++*p_argv)[0];

        if (!strcmp(opt, "-qemu")) {
            --(*p_argc);
            ++(*p_argv);
            break;
        }

        int helpStatus = emulator_parseHelpOption(opt);
        if (helpStatus >= 0) {
            *exit_status = helpStatus;
            return false;
        }

        if (opt[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", opt);
            fprintf(stderr, "please use -help for a list of valid options\n");
            return false;
        }

        fprintf(stderr, "invalid command-line parameter: %s.\n", opt);
        fprintf(stderr,
                "Hint: use '@foo' to launch a virtual device named 'foo'.\n");
        fprintf(stderr, "please use -help for more information\n");
        return false;
    }

    if (opts->version) {
        printf("Android emulator version %s (CL:%s)\n"
               "Copyright (C) 2006-2017 The Android Open Source Project and "
               "many "
               "others.\n"
               "This program is a derivative of the QEMU CPU emulator "
               "(www.qemu.org).\n\n",
#if defined ANDROID_SDK_TOOLS_BUILD_NUMBER
               EMULATOR_VERSION_STRING
               " (build_id " STRINGIFY(ANDROID_SDK_TOOLS_BUILD_NUMBER) ")",
#else
               EMULATOR_VERSION_STRING,
#endif
               EMULATOR_CL_SHA1);

        printf("  This software is licensed under the terms of the GNU General "
               "Public\n"
               "  License version 2, as published by the Free Software "
               "Foundation, and\n"
               "  may be copied, distributed, and modified under those "
               "terms.\n\n"
               "  This program is distributed in the hope that it will be "
               "useful,\n"
               "  but WITHOUT ANY WARRANTY; without even the implied warranty "
               "of\n"
               "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See "
               "the\n"
               "  GNU General Public License for more details.\n\n");

        *exit_status = 0;
        return false;
    }

    if (opts->webcam_list) {
        android_camera_list_webcams();
        *exit_status = 0;
        return false;
    }

    if (opts->snapshot_list) {
        bool android_snapshot_list_ok = false;

        if (opts->snapstorage == NULL) {
            /* Need to find the default snapstorage */
            int inAndroidBuild = 0;
            AvdInfo* avd = createAVD(opts, &inAndroidBuild);
            str_reset(&opts->snapstorage, avdInfo_getSnapStoragePath(avd));
            avdInfo_free(avd);
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            } else {
                if (inAndroidBuild) {
                    derror("You must use the -snapstorage <file> option to "
                           "specify a snapshot storage file!\n");
                } else {
                    if (is_qemu2) {
                        android_snapshot_list_ok = true;
                    } else {
                        return false;
                    }
                }
                if (!android_snapshot_list_ok) {
                    return false;
                }
            }
        }
        if (!android_snapshot_list_ok) {
            snapshot_print(opts->snapstorage);
            *exit_status = 0;
            return false;
        }
    }

    // Both |argc| and |argv| have been modified by the big while loop above:
    // |argc| should now be the number of options after '-qemu', and if that is
    // positive, |argv| should point to the first option following '-qemu'.
    // Now we check if any of these QEMU options is an 'informational' option,
    // e.g. '-h', '-version', etc.
    // The extra pair of parentheses is to keep gcc happy.
    char* qemu_info_opt = _findQemuInformationalOption(*p_argc, *p_argv);
    if (qemu_info_opt) {
        D("Found informational option '%s' after '-qemu'.\n"
          "All options before '-qemu' will be ignored!",
          qemu_info_opt);
        *exit_status = EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER;
        return false;
    }

    sanitizeOptions(opts);

    if (opts->selinux) {
        if ((strcmp(opts->selinux, "permissive") != 0) &&
            (strcmp(opts->selinux, "disabled") != 0)) {
            derror("-selinux must be \"disabled\" or \"permissive\"");
            return false;
        }
    }

    /* Parses options and builds an appropriate AVD. */
    int inAndroidBuild = 0;
    AvdInfo* avd = *the_avd = createAVD(opts, &inAndroidBuild);

    /* get the skin from the virtual device configuration */
    if (opts->skindir != NULL) {
        if (opts->skin == NULL) {
            /* NOTE: Normally handled by sanitizeOptions(), just be safe */
            derror("The -skindir <path> option requires a -skin <name> option");
            *exit_status = 2;
            return false;
        }
    } else {
        char* skinName;
        char* skinDir;

        avdInfo_getSkinInfo(avd, &skinName, &skinDir);

        if (opts->skin == NULL) {
            str_reset_nocopy(&opts->skin, skinName);
            D("autoconfig: -skin %s", opts->skin);
        } else {
            AFREE(skinName);
        }

        str_reset_nocopy(&opts->skindir, skinDir);
        D("autoconfig: -skindir %s", opts->skindir);
    }
    /* update the avd hw config from this new skin */
    avdInfo_getSkinHardwareIni(avd, opts->skin, opts->skindir);

    if (avdInfo_initHwConfig(avd, hw, is_qemu2) < 0) {
        derror("could not read hardware configuration ?");
        return false;
    }

    // Make the parsed avd globally available,
    // The features below sneakily rely on avd info being availalbe)
    getConsoleAgents()->settings->inject_AvdInfo(avd);
    // Update server-based hw config / feature flags.
    // Must be done after createAVD,  which sets up critical info needed
    // by featurecontrol component itself.
#if (SNAPSHOT_PROFILE > 1)
    dprint("Starting feature flag application and host hw query with uptime "
           "%" PRIu64 " ms",
           get_uptime_ms());
#endif
    feature_initialize();
    feature_update_from_server();
#if (SNAPSHOT_PROFILE > 1)
    dprint("Finished feature flag application and host hw query with uptime "
           "%" PRIu64 " ms",
           get_uptime_ms());
#endif

    if (feature_is_enabled(kFeature_Minigbm) &&
        strcmp(hw->hw_gltransport, "virtio-gpu-asg") &&
        strcmp(hw->hw_gltransport, "virtio-gpu-pipe")) {
        str_reset(&hw->hw_gltransport, "virtio-gpu-pipe");
        D("force gltransport to virtio-gpu-pipe\n");
    }

    if (opts->acpi_config != NULL) {
        if (!path_exists(opts->acpi_config)) {
            derror("Invalid ACPI config file path: %s", opts->acpi_config);
            return false;
        } else {
            avdInfo_setAcpiIniPath(avd, opts->acpi_config);
        }
    }

    emulator_has_network_option =
            (opts->netspeed && (opts->netspeed[0] != '\0')) ||
            (opts->netdelay && (opts->netdelay[0] != '\0')) || opts->netfast;

    if (!opts->netspeed && android_skin_net_speed) {
        D("skin network speed: '%s'", android_skin_net_speed);
        if (strcmp(android_skin_net_speed, NETWORK_SPEED_DEFAULT) != 0) {
            str_reset(&opts->netspeed, android_skin_net_speed);
        }
    }

    if (!opts->netdelay && android_skin_net_delay) {
        D("skin network delay: '%s'", android_skin_net_delay);
        if (strcmp(android_skin_net_delay, NETWORK_DELAY_DEFAULT) != 0) {
            str_reset(&opts->netdelay, android_skin_net_delay);
        }
    }

    /* Initialize net speed and delays stuff. */
    if (!android_network_set_speed(opts->netspeed)) {
        derror("invalid -netspeed parameter '%s'", opts->netspeed);
        return false;
    }

    if (!android_network_set_latency(opts->netdelay)) {
        derror("invalid -netdelay parameter '%s'", opts->netdelay);
        return false;
    }

    if (opts->netfast) {
        android_network_set_speed("full");
        android_network_set_latency("none");
    }

    if (opts->code_profile) {
        char* profilePath = avdInfo_getCodeProfilePath(avd, opts->code_profile);
        if (profilePath == NULL) {
            derror("bad -code-profile parameter");
            return false;
        }
        int ret = path_mkdir_if_needed(profilePath, 0755);
        if (ret < 0) {
            derror("could not create directory '%s'\n", profilePath);
            *exit_status = 2;
            return false;
        }
        str_reset_nocopy(&opts->code_profile, profilePath);
    }

    if (opts->unix_pipe) {
    }

    // Update CPU architecture for HW configs created from build directory.
    if (inAndroidBuild) {
        str_reset(&hw->hw_cpu_arch, targetArch);
    }

    if (!emulator_handleCommonEmulatorOptions(opts, hw, avd, is_qemu2)) {
        return false;
    }

    /** SNAPSHOT STORAGE HANDLING */
    if (hw->fastboot_forceColdBoot) {
        D("autoconfig: -no-snapshot from AVD config.ini");
        opts->no_snapshot = 1;
    }

    /* -no-snapshot is equivalent to using both -no-snapshot-load
     * and -no-snapshot-save. You can still load/save snapshots dynamically
     * from the console though.
     */
    if (opts->no_snapshot) {
        opts->no_snapshot_load = 1;
        opts->no_snapshot_save = 1;
        if (opts->snapshot) {
            dwarning(
                    "ignoring -snapshot option due to the use of "
                    "-no-snapshot.");
        }
    }

    /* Determine snapstorage path. -no-snapstorage disables all snapshotting
     * support. This means you can't resume a snapshot at load, save it at
     * exit, or even load/save them dynamically at runtime with the console.
     */
    if (opts->no_snapstorage) {
        if (opts->snapshot) {
            dwarning(
                    "ignoring -snapshot option due to the use of "
                    "-no-snapstorage");
            str_reset_null(&opts->snapshot);
        }

        if (opts->snapstorage) {
            dwarning(
                    "ignoring -snapstorage option due to the use of "
                    "-no-snapstorage");
            str_reset_null(&opts->snapstorage);
        }

        // Android Studio used to pass 'no-snapstorage' when it actually meant
        // 'no-snapshot-load' - so treat it that way.
        opts->no_snapshot_load = 1;
    } else {
        if (!opts->snapstorage && avdInfo_getSnapshotPresent(avd)) {
            str_reset_nocopy(&opts->snapstorage,
                             avdInfo_getSnapStoragePath(avd));
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }

        if (opts->snapstorage && !path_exists(opts->snapstorage)) {
            D("no image at '%s', state snapshots disabled", opts->snapstorage);
            str_reset_null(&opts->snapstorage);
        }
    }

    /* If we have a valid snapshot storage path */
    if (opts->snapstorage) {
        if (is_qemu2) {
            dwarning(
                    "QEMU2 does not support legacy snapshots - option will be "
                    "ignored.");
        } else {
            // QEMU2 does not support some of the flags below, and the emulator
            // will fail to start if they are passed in, so for now, ignore them
            str_reset(&hw->disk_snapStorage_path, opts->snapstorage);

            if (!opts->no_snapshot_load || !opts->no_snapshot_save) {
                if (opts->snapshot == NULL) {
                    str_reset(&opts->snapshot, "default-boot");
                    D("autoconfig: -snapshot %s", opts->snapshot);
                }
            }
        }
    }

    if (!opts->logcat || opts->logcat[0] == 0) {
        const char* env = getenv("ANDROID_LOG_TAGS");
        if (env && env[0]) {
            str_reset(&opts->logcat, env);
        } else {
            str_reset_null(&opts->logcat);
        }
    }

    /* XXXX: TODO: implement -shell and -logcat through qemud instead */
    if (!opts->shell_serial) {
#ifdef _WIN32
        str_reset(&opts->shell_serial, "con:");
#else
        str_reset(&opts->shell_serial, "stdio");
#endif
    } else {
        opts->shell = 1;
    }

    if (hw->vm_heapSize == 0) {
        /* Compute the default heap size based on the RAM size.
         * Essentially, we want to ensure the following liberal mappings:
         *
         *   96MB RAM -> 16MB heap
         *  128MB RAM -> 24MB heap
         *  256MB RAM -> 48MB heap
         */
        int ramSize = hw->hw_ramSize;
        int heapSize;

        if (ramSize < 100)
            heapSize = 16;
        else if (ramSize < 192)
            heapSize = 24;
        else
            heapSize = 48;

        hw->vm_heapSize = heapSize;
    }

    str_reset(&hw->avd_name, avdInfo_getName(avd));
    str_reset(&hw->avd_id, avdInfo_getId(avd));

    /* Setup screen emulation */
    if (opts->screen) {
        if (strcmp(opts->screen, "touch") &&
            strcmp(opts->screen, "multi-touch") &&
            strcmp(opts->screen, "no-touch")) {
            derror("Invalid value for -screen <mode> parameter: %s\n"
                   "Valid values are: touch, multi-touch, or no-touch\n",
                   opts->screen);
            return false;
        }
        // BUG(140563470) Explicitly set screen to multi-touch when VirtioInput
        // feature is enabled because virtio input device is configured to be
        // multi-touch.
        if (strcmp(opts->screen, "touch") == 0 &&
            feature_is_enabled(kFeature_VirtioInput)) {
            str_reset(&hw->hw_screen, "multi-touch");
        } else {
            str_reset(&hw->hw_screen, opts->screen);
        }
    }

    // DNS server support. Validate the option if any, find the DNS
    // server IP addresses, then rewrite the option value with the
    // results.
    if (is_qemu2 && opts->dns_server) {
        // When dns server is provided after '-dns-server', qemu2 handles it in
        // the glue code (see android-qemu2-glue/qemu-setup-dns-servers.cpp).
        // If there are no dns servers provided for qemu2, take the dns servers
        // of the host as fall back.
    } else {
        // qemu1: QEMU1 only supports IPv4 DNS servers, so use
        //  android_dns_get_servers() that filters out any IPv6 server
        //  from the -dns-server list.
        // qemu2: the opts->dns_server is NULL, just get the dns servers of the
        //  host.
        SockAddress dnsServers[ANDROID_MAX_DNS_SERVERS] = {};
        int dnsCount = android_dns_get_servers(opts->dns_server, dnsServers);
        if (dnsCount < 0) {
            return false;
        }
        if (dnsCount > 0) {
            // If there's at least one server, rewrite the option with
            // its IP(s). Otherwise our engine is on its own.
            STRALLOC_DEFINE(newOption);
            int n;
            for (n = 0; n < dnsCount; ++n) {
                char tmp[256];
                stralloc_add_format(newOption, "%s%s", (n > 0) ? "," : "",
                                    sock_address_host_string(&dnsServers[n],
                                                             tmp, sizeof(tmp)));
            }
            str_reset(&opts->dns_server, stralloc_cstr(newOption));
            stralloc_reset(newOption);
        }
    }

    // Virtual CPU core count.
    if (opts->cores) {
        if (is_qemu2) {
            char* end = NULL;
            long val = strtol(opts->cores, &end, 10);
            if (*end != '\0' || val <= 0 || val > 64) {
                derror("Invalid value for -cores <count> parameter: %s\n",
                       "Valid values are decimal count between 1 and 64\n",
                       opts->cores);
                return false;
            }
            hw->hw_cpu_ncore = (int)val;
        } else {
            dwarning(
                    "Classic QEMU doesn't support -cores option, only one\n"
                    "virtual CPU can be emulated with this engine.\n");
            str_reset_null(&opts->cores);
        }
    }

    // Unix pipe paths
    {
        ParamList* opt = opts->unix_pipe;
        while (opt) {
            android_unix_pipes_add_allowed_path(opt->param);
            opt = opt->next;
        }
    }

    if (opts->phone_number) {
        char phone_number[16];
        int ret = validate_and_parse_phone_number(opts->phone_number,
                                                  phone_number);
        if (ret) {
            *exit_status = 2;
            return false;
        }
        str_reset(&opts->phone_number, phone_number);
    }

    *exit_status = 0;
    return true;
}

bool emulator_parseFeatureCommandLineOptions(AndroidOptions* opts,
                                             AvdInfo* avd,
                                             AndroidHwConfig* hw) {
    bool supportsVirtualScene = feature_is_enabled(kFeature_VirtualScene);
    bool supportsVideoPlayback = feature_is_enabled(kFeature_VideoPlayback);

    if (opts->camera_back) {
        bool isVirtualScene = !strcmp(opts->camera_back, "virtualscene");
        bool isVideoPlayback = !strcmp(opts->camera_back, "videoplayback");
        /* Validate parameter. */
        if (memcmp(opts->camera_back, "webcam", 6) &&
            strcmp(opts->camera_back, "emulated") &&
            (!isVirtualScene || !supportsVirtualScene) &&
            (!isVideoPlayback || !supportsVideoPlayback) &&
            strcmp(opts->camera_back, "none")) {
            derror("Invalid value for -camera-back <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>'%s%s, "
                   "or 'none'\n",
                   opts->camera_back,
                   supportsVirtualScene ? ", 'virtualscene'" : "",
                   supportsVideoPlayback ? ", 'videoplayback'" : "");
            return false;
        }
        str_reset(&hw->hw_camera_back, opts->camera_back);
    }

    if (opts->camera_front) {
        bool isVideoPlayback = !strcmp(opts->camera_front, "videoplayback");
        /* Validate parameter. */
        if (memcmp(opts->camera_front, "webcam", 6) &&
            strcmp(opts->camera_front, "emulated") &&
            (!isVideoPlayback || !supportsVideoPlayback) &&
            strcmp(opts->camera_front, "none")) {
            derror("Invalid value for -camera-front <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>'%s, "
                   "or 'none'\n",
                   opts->camera_front,
                   supportsVideoPlayback ? ", 'videoplayback'" : "");
            return false;
        }
        str_reset(&hw->hw_camera_front, opts->camera_front);
    }

    return true;
}

static RendererConfig lastRendererConfig;

static bool isGuestRendererChoice(const char* choice) {
    return choice && (!strcmp(choice, "off") || !strcmp(choice, "guest"));
}
