/* Copyright (C) 2011 The Android Open Source Project
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

#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/cpu_accelerator.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/globals.h"
#include "android/kernel/kernel_utils.h"
#include "android/help.h"
#include "android/opengl/emugl_config.h"
#include "android/resource.h"
#include "android/skin/charmap.h"
#include "android/snapshot.h"
#include "android/user-config.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/host_bitness.h"
#include "android/utils/path.h"
#include "android/utils/stralloc.h"
#include "android/utils/x86_cpuid.h"
#include "android/version.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef _WIN32
#include <process.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


const char*  skin_network_speed = NULL;
const char*  skin_network_delay = NULL;

/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****            U T I L I T Y   R O U T I N E S                  *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

// TODO(digit): Remove this!
// The plan is to move the -wipe-data and -writable-system feature to the
// top-level 'emulator' launcher program, so that the engines don't have
// to meddle with partition images, except for mounting them. The alternative
// is to add new QEMU1 and QEMU2 options to pass the corresponding flags,
// which is overkill, given this plan.
bool android_op_wipe_data = false;
bool android_op_writable_system = false;

void reassign_string(char** string, const char* new_value) {
    free(*string);
    *string = ASTRDUP(new_value);
}

#define ONE_MB (1024 * 1024)

unsigned convertBytesToMB( uint64_t  size )
{
    if (size == 0)
        return 0;

    size = (size + ONE_MB-1) >> 20;
    if (size > UINT_MAX)
        size = UINT_MAX;

    return (unsigned) size;
}

uint64_t convertMBToBytes( unsigned  megaBytes )
{
    return ((uint64_t)megaBytes << 20);
}

/* Return the full file path of |fileName| relative to |rootPath|.
 * Return a new heap-allocated string to be freed by the caller. Note that
 * if |fileName| is an absolute path, the function returns a copy and ignores
 * |rootPath|.
 */
static char*
_getFullFilePath(const char* rootPath, const char* fileName) {
    if (path_is_absolute(fileName)) {
        return ASTRDUP(fileName);
    } else {
        char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

        p = bufprint(temp, end, "%s/%s", rootPath, fileName);
        if (p >= end) {
            return NULL;
        }
        return ASTRDUP(temp);
    }
}

static uint64_t
_adjustPartitionSize( const char*  description,
                      uint64_t     imageBytes,
                      uint64_t     defaultBytes,
                      int          inAndroidBuild )
{
    char      temp[64];
    unsigned  imageMB;
    unsigned  defaultMB;

    if (imageBytes <= defaultBytes)
        return defaultBytes;

    imageMB   = convertBytesToMB(imageBytes);
    defaultMB = convertBytesToMB(defaultBytes);

    if (imageMB > defaultMB) {
        snprintf(temp, sizeof temp, "(%d MB > %d MB)", imageMB, defaultMB);
    } else {
        snprintf(temp, sizeof temp, "(%" PRIu64 "  bytes > %" PRIu64 " bytes)", imageBytes, defaultBytes);
    }

    if (inAndroidBuild) {
        dwarning("%s partition size adjusted to match image file %s\n", description, temp);
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
static char*
_getSdkImagePath( const char*  fileName )
{
    char   temp[MAX_PATH];
    char*  p   = temp;
    char*  end = p + sizeof(temp);
    char*  q;
    char*  app;

    static const char* const  searchPaths[] = {
        "",                                  /* program's directory */
        "/lib/images",                       /* this is for SDK 1.0 */
        "/../platforms/android-1.1/images",  /* this is for SDK 1.1 */
        NULL
    };

    app = bufprint_app_dir(temp, end);
    if (app >= end)
        return NULL;

    do {
        int  nn;

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
            DirScanner*  scanner = dirScanner_new(temp);
            if (scanner != NULL) {
                int          found = 0;
                const char*  subdir;

                for (;;) {
                    subdir = dirScanner_next(scanner);
                    if (!subdir) break;

                    q = bufprint(p, end, "/%s/images/%s", subdir, fileName);
                    if (q >= end || !path_exists(temp))
                        continue;

                    found = 1;
                    p = bufprint(p, end, "/%s/images", subdir);
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
    //D("image auto-detection: %s/%s", temp, fileName);
    return android_strdup(temp);
}

static char*
_getSdkImage( const char*  path, const char*  file )
{
    char  temp[MAX_PATH];
    char  *p = temp, *end = p + sizeof(temp);

    p = bufprint(temp, end, "%s/%s", path, file);
    if (p >= end || !path_exists(temp))
        return NULL;

    return android_strdup(temp);
}

static char*
_getSdkSystemImage( const char*  path, const char*  optionName, const char*  file )
{
    char*  image = _getSdkImage(path, file);

    if (image == NULL) {
        derror("Your system directory is missing the '%s' image file.\n"
               "Please specify one with the '%s <filepath>' option",
               file, optionName);
        exit(2);
    }
    return image;
}

// Sanitize options |opts|. Return 0 on success, or positive exit status code on error.
static int sanitizeOptions(AndroidOptions* opts) {
    /* legacy support: we used to use -system <dir> and -image <file>
     * instead of -sysdir <dir> and -system <file>, so handle this by checking
     * whether the options point to directories or files.
     */
    if (opts->image != NULL) {
        if (opts->system != NULL) {
            if (opts->sysdir != NULL) {
                derror( "You can't use -sysdir, -system and -image at the same time.\n"
                        "You should probably use '-sysdir <path> -system <file>'.\n" );
                return 2;
            }
        }
        dwarning( "Please note that -image is obsolete and that -system is now used to point\n"
                  "to the system image. Next time, try using '-sysdir <path> -system <file>' instead.\n" );
        opts->sysdir = opts->system;
        opts->system = opts->image;
        opts->image  = NULL;
    }
    else if (opts->system != NULL && path_is_dir(opts->system)) {
        if (opts->sysdir != NULL) {
            derror( "Option -system should now be followed by a file path, not a directory one.\n"
                    "Please use '-sysdir <path>' to point to the system directory.\n" );
            return 1;
        }
        dwarning( "Please note that the -system option should now be used to point to the initial\n"
                  "system image (like the obsolete -image option). To point to the system directory\n"
                  "please now use '-sysdir <path>' instead.\n" );

        opts->sysdir = opts->system;
        opts->system = NULL;
    }

    if (opts->nojni) {
        opts->no_jni = opts->nojni;
        opts->nojni  = 0;
    }

    if (opts->nocache) {
        opts->no_cache = opts->nocache;
        opts->nocache  = 0;
    }

    if (opts->noaudio) {
        opts->no_audio = opts->noaudio;
        opts->noaudio  = 0;
    }

    if (opts->noskin) {
        opts->no_skin = opts->noskin;
        opts->noskin  = 0;
    }

    /* If -no-cache is used, ignore any -cache argument */
    if (opts->no_cache) {
        opts->cache = 0;
    }

    /* the purpose of -no-audio is to disable sound output from the emulator,
     * not to disable Audio emulation. So simply force the 'none' backends */
    if (opts->no_audio)
        opts->audio = "none";

    /* we don't accept -skindir without -skin now
     * to simplify the autoconfig stuff with virtual devices
     */
    if (opts->no_skin) {
        opts->skin    = "320x480";
        opts->skindir = NULL;
    }

    if (opts->skindir) {
        if (!opts->skin) {
            derror( "the -skindir <path> option requires a -skin <name> option");
            return 1;
        }
    }

    if (opts->bootchart) {
        char*  end;
        int    timeout = strtol(opts->bootchart, &end, 10);
        if (timeout == 0)
            opts->bootchart = NULL;
        else if (timeout < 0 || timeout > 15*60) {
            derror( "timeout specified for -bootchart option is invalid.\n"
                    "please use integers between 1 and 900\n");
            return 1;
        }
    }
    return 0;
}

AvdInfo* createAVD(AndroidOptions* opts, int* inAndroidBuild)
{
    AvdInfo* ret = NULL;
    char   tmp[MAX_PATH];
    char*  tmpend = tmp + sizeof(tmp);
    char*  android_build_root = NULL;
    char*  android_build_out  = NULL;

    /* If no AVD name was given, try to find the top of the
     * Android build tree
     */
    if (opts->avd == NULL) {
        do {
            char*  out = getenv("ANDROID_PRODUCT_OUT");

            if (out == NULL || out[0] == 0)
                break;

            if (!path_exists(out)) {
                derror("Can't access ANDROID_PRODUCT_OUT as '%s'\n"
                    "You need to build the Android system before launching the emulator",
                    out);
                exit(2);
            }

            android_build_root = getenv("ANDROID_BUILD_TOP");
            if (android_build_root == NULL || android_build_root[0] == 0)
                break;

            if (!path_exists(android_build_root)) {
                derror("Can't find the Android build root '%s'\n"
                    "Please check the definition of the ANDROID_BUILD_TOP variable.\n"
                    "It should point to the root of your source tree.\n",
                    android_build_root );
                exit(2);
            }
            android_build_out = out;
            D( "found Android build root: %s", android_build_root );
            D( "found Android build out:  %s", android_build_out );
        } while (0);
    }
    /* if no virtual device name is given, and we're not in the
     * Android build system, we'll need to perform some auto-detection
     * magic :-)
     */
    if (opts->avd == NULL && !android_build_out)
    {
        if (!opts->sysdir) {
            opts->sysdir = _getSdkImagePath("system.img");
            if (!opts->sysdir) {
                derror(
                "You did not specify a virtual device name, and the system\n"
                "directory could not be found.\n\n"
                "If you are an Android SDK user, please use '@<name>' or '-avd <name>'\n"
                "to start a given virtual device (use -list-avds to print available ones).\n\n"

                "Otherwise, follow the instructions in -help-disk-images to start the emulator\n"
                );
                exit(2);
            }
            D("autoconfig: -sysdir %s", opts->sysdir);
        }

        if (!opts->system) {
            opts->system = _getSdkSystemImage(opts->sysdir, "-image", "system.img");
            D("autoconfig: -system %s", opts->system);
        }

        if (!opts->kernel) {
            opts->kernel = _getSdkSystemImage(opts->sysdir, "-kernel", "kernel-qemu");
            D("autoconfig: -kernel %s", opts->kernel);
        }

        if (!opts->ramdisk) {
            opts->ramdisk = _getSdkSystemImage(opts->sysdir, "-ramdisk", "ramdisk.img");
            D("autoconfig: -ramdisk %s", opts->ramdisk);
        }

        /* if no data directory is specified, use the system directory */
        if (!opts->datadir) {
            opts->datadir   = android_strdup(opts->sysdir);
            D("autoconfig: -datadir %s", opts->sysdir);
        }

        if (!opts->data) {
            /* check for userdata-qemu.img in the data directory */
            bufprint(tmp, tmpend, "%s/userdata-qemu.img", opts->datadir);
            if (!path_exists(tmp)) {
                derror(
                "You did not provide the name of an Android Virtual Device\n"
                "with the '-avd <name>' option. Read -help-avd for more information.\n\n"

                "If you *really* want to *NOT* run an AVD, consider using '-data <file>'\n"
                "to specify a data partition image file (I hope you know what you're doing).\n"
                );
                exit(2);
            }

            opts->data = android_strdup(tmp);
            D("autoconfig: -data %s", opts->data);
        }

        if (!opts->snapstorage && opts->datadir) {
            bufprint(tmp, tmpend, "%s/snapshots.img", opts->datadir);
            if (path_exists(tmp)) {
                opts->snapstorage = android_strdup(tmp);
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }
    }

    /* setup the virtual device differently depending on whether
     * we are in the Android build system or not
     */
    if (opts->avd != NULL)
    {
        ret = avdInfo_new( opts->avd, android_avdParams );
        if (ret == NULL) {
            /* an error message has already been printed */
            dprint("could not find virtual device named '%s'", opts->avd);
            exit(1);
        }
    }
    else
    {
        if (!android_build_out) {
            android_build_out = android_build_root = opts->sysdir;
        }
        ret = avdInfo_newForAndroidBuild(
                            android_build_root,
                            android_build_out,
                            android_avdParams );

        if(ret == NULL) {
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

// Handle some common emulator options. Return 0 on success, or
// status exit code > 0 on failure.
static int handleCommonEmulatorOptions(AndroidOptions* opts,
                                       AndroidHwConfig* hw,
                                       AvdInfo* avd) {
    int forceArmv7 = 0;

    // Kernel options
    {
        char*  kernelFile    = opts->kernel;
        int    kernelFileLen;

        if (kernelFile == NULL) {
            kernelFile = opts->ranchu ?
                    avdInfo_getRanchuKernelPath(avd) :
                    avdInfo_getKernelPath(avd);
            if (kernelFile == NULL) {
                derror( "This AVD's configuration is missing a kernel file!!" );
                const char* sdkRootDir = getenv("ANDROID_SDK_ROOT");
                if (sdkRootDir) {
                    derror( "ANDROID_SDK_ROOT is defined (%s) but cannot find kernel file in "
                            "%s" PATH_SEP "system-images" PATH_SEP
                            " sub directories", sdkRootDir, sdkRootDir);
                } else {
                    derror( "ANDROID_SDK_ROOT is undefined");
                }
                return 2;
            }
            D("autoconfig: -kernel %s", kernelFile);
        }
        if (!path_exists(kernelFile)) {
            derror( "Invalid or missing kernel image file: %s", kernelFile );
            return 2;
        }

        hw->kernel_path = kernelFile;

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
         * some build system changes. I prefer not to do that for now for reasons
         * of simplicity.
         */
         kernelFileLen = strlen(kernelFile);
         if (kernelFileLen > 6 && !memcmp(kernelFile + kernelFileLen - 6, "-armv7", 6)) {
             forceArmv7 = 1;
         }
    }

    /* If the target ABI is armeabi-v7a, we can auto-detect the cpu model
     * as a cortex-a8, instead of the default (arm926) which only emulates
     * an ARMv5TE CPU.
     */
    if (!forceArmv7 && hw->hw_cpu_model[0] == '\0')
    {
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
            reassign_string(&hw->hw_cpu_model, "qemu32");
            D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
        }
        AFREE(arch);
    }

    if (forceArmv7 != 0) {
        reassign_string(&hw->hw_cpu_model, "cortex-a8");
        D("Auto-config: -qemu -cpu %s", hw->hw_cpu_model);
    }

    char versionString[256];
    if (!android_pathProbeKernelVersionString(hw->kernel_path,
                                              versionString,
                                              sizeof(versionString))) {
        derror("Can't find 'Linux version ' string in kernel image file: %s",
               hw->kernel_path);
        return 2;
    }

    KernelVersion kernelVersion = 0;
    if (!android_parseLinuxVersionString(versionString, &kernelVersion)) {
        derror("Can't parse 'Linux version ' string in kernel image file: '%s'",
               versionString);
        return 2;
    }

    // Auto-detect kernel device naming scheme if needed.
    if (androidHwConfig_getKernelDeviceNaming(hw) < 0) {
        const char* newDeviceNaming = "no";
        if (kernelVersion >= KERNEL_VERSION_3_10_0) {
            D("Auto-detect: Kernel image requires new device naming scheme.");
            newDeviceNaming = "yes";
        } else {
            D("Auto-detect: Kernel image requires legacy device naming scheme.");
        }
        reassign_string(&hw->kernel_newDeviceNaming, newDeviceNaming);
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
        reassign_string(&hw->kernel_supportsYaffs2, newYaffs2Support);
    }

    /* opts->ramdisk is never NULL (see createAVD) here */
    if (opts->ramdisk) {
        reassign_string(&hw->disk_ramdisk_path, opts->ramdisk);
    }
    else if (!hw->disk_ramdisk_path[0]) {
        hw->disk_ramdisk_path = avdInfo_getRamdiskPath(avd);
        D("autoconfig: -ramdisk %s", hw->disk_ramdisk_path);
    }

    /* -partition-size is used to specify the max size of both the system
     * and data partition sizes.
     */
    uint64_t defaultPartitionSize = convertMBToBytes(200);

    if (opts->partition_size) {
        char*  end;
        long   sizeMB = strtol(opts->partition_size, &end, 0);
        long   minSizeMB = 10;
        long   maxSizeMB = LONG_MAX / ONE_MB;

        if (sizeMB < 0 || *end != 0) {
            derror( "-partition-size must be followed by a positive integer" );
            return 1;
        }
        if (sizeMB < minSizeMB || sizeMB > maxSizeMB) {
            derror( "partition-size (%d) must be between %dMB and %dMB",
                    sizeMB, minSizeMB, maxSizeMB );
            return 1;
        }
        defaultPartitionSize = (uint64_t) sizeMB * ONE_MB;
    }


    /** SYSTEM PARTITION **/

    if (opts->sysdir == NULL) {
        if (avdInfo_inAndroidBuild(avd)) {
            opts->sysdir = ASTRDUP(avdInfo_getContentPath(avd));
            D("autoconfig: -sysdir %s", opts->sysdir);
        }
    }

    if (opts->sysdir != NULL) {
        if (!path_exists(opts->sysdir)) {
            derror("Directory does not exist: %s", opts->sysdir);
            return 1;
        }
    }

    {
        char*  rwImage   = NULL;
        char*  initImage = NULL;

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
            hw->disk_systemPartition_path     = rwImage;
            hw->disk_systemPartition_initPath = NULL;
            D("Using direct system image: %s", rwImage);
        } else if (initImage != NULL) {
            hw->disk_systemPartition_path = NULL;
            hw->disk_systemPartition_initPath = initImage;
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
        uint64_t    systemBytes;

        if (systemImage == NULL)
            systemImage = hw->disk_systemPartition_initPath;

        if (path_get_size(systemImage, &systemBytes) < 0) {
            derror("Missing system image: %s", systemImage);
            return 1;
        }

        hw->disk_systemPartition_size =
            _adjustPartitionSize("system", systemBytes, defaultPartitionSize,
                                 avdInfo_inAndroidBuild(avd));
    }

    /** DATA PARTITION **/

    if (opts->datadir) {
        if (!path_exists(opts->datadir)) {
            derror("Invalid -datadir directory: %s", opts->datadir);
            return 1;
        }
    }

    {
        char*  dataImage = NULL;
        char*  initImage = NULL;

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
                    return 1;
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
                return 1;
            }
        } else {
            initImage = avdInfo_getDataInitImagePath(avd);
            D("autoconfig: -initdata %s", initImage);
        }

        hw->disk_dataPartition_path = dataImage;
        if (opts->wipe_data) {
            hw->disk_dataPartition_initPath = initImage;
        } else {
            hw->disk_dataPartition_initPath = NULL;
        }
        android_op_wipe_data = opts->wipe_data;
        android_op_writable_system = opts->writable_system;

        uint64_t     defaultBytes =
                hw->disk_dataPartition_size == 0 ?
                defaultPartitionSize :
                hw->disk_dataPartition_size;
        uint64_t     dataBytes;
        const char*  dataPath = hw->disk_dataPartition_initPath;

        if (dataPath == NULL)
            dataPath = hw->disk_dataPartition_path;

        path_get_size(dataPath, &dataBytes);

        hw->disk_dataPartition_size =
            _adjustPartitionSize("data", dataBytes, defaultBytes,
                                 avdInfo_inAndroidBuild(avd));
    }

    /** CACHE PARTITION **/

    if (opts->no_cache) {
        /* No cache partition at all */
        hw->disk_cachePartition = 0;
    }
    else if (!hw->disk_cachePartition) {
        if (opts->cache) {
            dwarning( "Emulated hardware doesn't support a cache partition. -cache option ignored!" );
            opts->cache = NULL;
        }
    }
    else
    {
        if (!opts->cache) {
            /* Find the current cache partition file */
            opts->cache = avdInfo_getCachePath(avd);
            if (opts->cache == NULL) {
                /* The file does not exists, we will force its creation
                 * if we are not in the Android build system. Otherwise,
                 * a temporary file will be used.
                 */
                if (!avdInfo_inAndroidBuild(avd)) {
                    opts->cache = avdInfo_getDefaultCachePath(avd);
                }
            }
            if (opts->cache) {
                D("autoconfig: -cache %s", opts->cache);
            }
        }

        if (opts->cache) {
            hw->disk_cachePartition_path = ASTRDUP(opts->cache);
        }
    }

    if (hw->disk_cachePartition_path && opts->cache_size) {
        /* Set cache partition size per user options. */
        char*  end;
        long   sizeMB = strtol(opts->cache_size, &end, 0);

        if (sizeMB < 0 || *end != 0) {
            derror( "-cache-size must be followed by a positive integer" );
            return 1;
        }
        hw->disk_cachePartition_size = (uint64_t) sizeMB * ONE_MB;
    }

    /** SD CARD PARTITION */

    if (!hw->hw_sdCard) {
        /* No SD Card emulation, so -sdcard will be ignored */
        if (opts->sdcard) {
            dwarning( "Emulated hardware doesn't support SD Cards. -sdcard option ignored." );
            opts->sdcard = NULL;
        }
    } else {
        /* Auto-configure -sdcard if it is not available */
        if (!opts->sdcard) {
            do {
                /* If -datadir <path> is used, look for a sdcard.img file here */
                if (opts->datadir) {
                    char tmp[PATH_MAX], *tmpend = tmp + sizeof(tmp);
                    bufprint(tmp, tmpend, "%s/%s", opts->datadir, "system.img");
                    if (path_exists(tmp)) {
                        opts->sdcard = strdup(tmp);
                        break;
                    }
                }

                /* Otherwise, look at the AVD's content */
                opts->sdcard = avdInfo_getSdCardPath(avd);
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

    if(opts->sdcard) {
        uint64_t  size;
        if (path_get_size(opts->sdcard, &size) == 0) {
            /* see if we have an sdcard image.  get its size if it exists */
            /* due to what looks like limitations of the MMC protocol, one has
             * to use an SD Card image that is equal or larger than 9 MB
             */
            if (size < 9*1024*1024ULL) {
                dwarning("SD Card files must be at least 9MB, ignoring '%s'\n", opts->sdcard);
            } else {
                hw->hw_sdCard_path = ASTRDUP(opts->sdcard);
            }
        } else {
            dwarning("no SD Card image at '%s'", opts->sdcard);
        }
    }

    if (opts->selinux) {
        if ((strcmp(opts->selinux, "permissive") != 0)
                && (strcmp(opts->selinux, "disabled") != 0)) {
            derror("-selinux must be \"disabled\" or \"permissive\"");
            return 1;
        }

        // SELinux 'disabled' mode is no longer supported starting with M.
        // See https://android-review.googlesource.com/#/c/148538/
        const int kSELinuxWithoutDisabledApiLevel = 23;
        if (!strcmp(opts->selinux, "disabled") &&
                avdInfo_getApiLevel(avd) >= kSELinuxWithoutDisabledApiLevel) {
            dwarning("SELinux 'disabled' is no longer supported starting "
                     "with API level %d, switching to 'permissive'",
                     kSELinuxWithoutDisabledApiLevel);
            opts->selinux = "permissive";
        }
    }

    if (opts->memory) {
        char*  end;
        long   ramSize = strtol(opts->memory, &end, 0);
        if (ramSize < 0 || *end != 0) {
            derror( "-memory must be followed by a positive integer" );
            return 1;
        }
        if (ramSize < 32 || ramSize > 4096) {
            derror( "physical memory size must be between 32 and 4096 MB" );
            return 1;
        }
        hw->hw_ramSize = ramSize;
    } else {
        int ramSize = hw->hw_ramSize;
        if (ramSize <= 0) {
            /* Compute the default RAM size based on the size of screen.
             * This is only used when the skin doesn't provide the ram
             * size through its hardware.ini (i.e. legacy ones) or when
             * in the full Android build system.
             */
            int64_t pixels  = hw->hw_lcd_width * hw->hw_lcd_height;
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
                ramSize = 96;
            else if (pixels <= 500000)
                ramSize = 128;
            else
                ramSize = 256;
        }
        hw->hw_ramSize = ramSize;
    }
#if defined(_WIN32) && !defined(_WIN64)
    const bool is32bitWindowsHost = true;
#else
    const bool is32bitWindowsHost = false;
#endif
    // For recent system versions, ensure a minimum of 1GB or memory, anything
    // lower is very painful during the boot process and after that.
    if (hw->hw_ramSize < 1024 && avdInfo_getApiLevel(avd) >= 21 && !is32bitWindowsHost) {
        dwarning("Increasing RAM size to 1GB");
        hw->hw_ramSize = 1024;
    }

    D("Physical RAM size: %dMB\n", hw->hw_ramSize);
    return 0;
}

bool handleCpuAcceleration(AndroidOptions* opts, AvdInfo* avd,
                           CpuAccelMode* accel_mode, char* accel_status) {
    /* Handle CPU acceleration options. */
    if (opts->no_accel) {
        if (opts->accel) {
            if (strcmp(opts->accel, "off") != 0) {
                derror("You cannot use -no-accel and '-accel %s' at the same time",
                       opts->accel);
                exit(1);
            }
        } else {
            reassign_string(&opts->accel, "off");
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
            derror("Invalid '-accel %s' parameter, valid values are: on off auto\n",
                   opts->accel);
            exit(1);
        }
    }

    AndroidCpuAcceleration accel_capability = androidCpuAcceleration_getStatus(&accel_status);
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
        dprint("CPU Acceleration status: %s", accel_status);
    }

    // Special case: x86/x86_64 emulation currently requires hardware
    // acceleration, so refuse to start in 'auto' mode if it is not
    // available.
    {
        char* abi = avdInfo_getTargetAbi(avd);
        if (!strncmp(abi, "x86", 3)) {
            if (!accel_ok && *accel_mode != ACCEL_OFF) {
                derror("%s emulation currently requires hardware acceleration!\n"
                    "Please ensure %s is properly installed and usable.\n"
                    "CPU acceleration status: %s",
                    abi, kAccelerator, accel_status);
                exit(1);
            }
            else if (*accel_mode == ACCEL_OFF) {
                // '-no-accel' of '-accel off' was used explicitly. Warn about
                // the issue but do not exit.
                dwarning("%s emulation may not work without hardware acceleration!", abi);
            }
            else {
                /* CPU acceleration is enabled and working, but if the host CPU
                 * does not support all instruction sets specified in the x86/
                 * x86_64 ABI, emulation may fail on unsupported instructions.
                 * Therefore, check the capabilities of the host CPU and warn
                 * the user if any required features are missing. */
                uint32_t ecx = 0;
                char buf[64], *p = buf, * const end = p + sizeof(buf);

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
                    fprintf(stderr, "emulator: WARNING: Host CPU is missing the"
                            " following feature(s) required for %s emulation:%s"
                            "\nHardware-accelerated emulation may not work"
                            " properly!\n", abi, buf);
                }
            }
        }
        AFREE(abi);
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

int parseEmulatorCommandLineOptions(int* pargc,
                                    char*** pargv,
                                    AndroidOptions* opts,
                                    AndroidHwConfig* hw,
                                    AvdInfo** theAvd) {
    *theAvd = NULL;

    // Invoke simple options parser to set |opts| properly.
    if (android_parse_options(pargc, pargv, opts) < 0) {
        return -1;
    }

    // Handle -help, -version and -qemu options here.
    while (*pargc > 1) {
        const char* opt = (++*pargv)[0];

        if (!strcmp(opt, "-qemu")) {
            (*pargc)--;
            (*pargv)++;
            break;
        }

        if (!strcmp(opt, "-help")) {
            STRALLOC_DEFINE(out);
            android_help_main(out);
            printf("%.*s", out->n, out->s);
            stralloc_reset(out);
            return 0;
        }

        if (!strncmp(opt, "-help-",6)) {
            STRALLOC_DEFINE(out);
            opt += 6;

            if (!strcmp(opt, "all")) {
                android_help_all(out);
            } else if (android_help_for_option(opt, out) == 0) {
                /* ok */
            } else if (android_help_for_topic(opt, out) == 0) {
                /* ok */
            }
            if (out->n > 0) {
                printf("\n%.*s", out->n, out->s);
                stralloc_reset(out);
                return 0;
            }

            fprintf(stderr, "unknown option: -help-%s\n", opt);
            fprintf(stderr, "please use -help for a list of valid topics\n");
            stralloc_reset(out);
            return -1;
        }

        if (opt[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", opt);
            fprintf(stderr, "please use -help for a list of valid options\n");
            return -1;
        }

        fprintf(stderr, "invalid command-line parameter: %s.\n", opt);
        fprintf(stderr, "Hint: use '@foo' to launch a virtual device named 'foo'.\n");
        fprintf(stderr, "please use -help for more information\n");
        return -1;
    }

    if (opts->version) {
      printf("Android emulator version %s\n"
             "Copyright (C) 2006-2015 The Android Open Source Project and many "
             "others.\n"
             "This program is a derivative of the QEMU CPU emulator "
             "(www.qemu.org).\n\n",
#if defined ANDROID_BUILD_ID
             VERSION_STRING " (build_id " STRINGIFY(ANDROID_BUILD_ID) ")");
#else
             VERSION_STRING);
#endif
        printf("  This software is licensed under the terms of the GNU General Public\n"
               "  License version 2, as published by the Free Software Foundation, and\n"
               "  may be copied, distributed, and modified under those terms.\n\n"
               "  This program is distributed in the hope that it will be useful,\n"
               "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
               "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
               "  GNU General Public License for more details.\n\n");

        return 0;
    }

    /* Both |argc| and |argv| have been modified by the big while loop above:
     * |argc| should now be the number of options after '-qemu', and if that is
     * positive, |argv| should point to the first option following '-qemu'.
     * Now we check if any of these QEMU options is an 'informational' option,
     * e.g. '-h', '-version', etc.
     * The extra pair of parentheses is to keep gcc happy.
     */
    char* qemu_info_opt = _findQemuInformationalOption(*pargc, *pargv);
    if (qemu_info_opt) {
        D("Found informational option '%s' after '-qemu'.\n"
          "All options before '-qemu' will be ignored!", qemu_info_opt);
        // NOTE: 2 is a special value, see documentation for this function.
        return 2;
    }

    if (opts->snapshot_list) {
        if (opts->snapstorage == NULL) {
            /* Need to find the default snapstorage */
            int inAndroidBuild = 0;
            AvdInfo* avd = createAVD(opts, &inAndroidBuild);
            opts->snapstorage = avdInfo_getSnapStoragePath(avd);
            avdInfo_free(avd);
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            } else {
                if (inAndroidBuild) {
                    derror("You must use the -snapstorage <file> option to specify a snapshot storage file!\n");
                } else {
                    derror("This AVD doesn't have snapshotting enabled!\n");
                }
                return -1;
            }
        }
        snapshot_print(opts->snapstorage);
        return 0;
    }

    int ret = sanitizeOptions(opts);
    if (ret) {
        return -ret;
    }

    /* Parses options and builds an appropriate AVD. */
    int inAndroidBuild = 0;
    AvdInfo* avd = android_avdInfo = createAVD(opts, &inAndroidBuild);

    *theAvd = avd;

    ret = handleCommonEmulatorOptions(opts, hw, avd);
    if (ret) {
        return -ret;
    }

    /* get the skin from the virtual device configuration */
    if (!opts->skindir) {
        char* skinName;
        char* skinDir;

        avdInfo_getSkinInfo(avd, &skinName, &skinDir);

        if (!opts->skin) {
            opts->skin = skinName;
            D("autoconfig: -skin %s", opts->skin);
        } else {
            AFREE(skinName);
        }

        opts->skindir = skinDir;
        D("autoconfig: -skindir %s", opts->skindir);
    }

    /* update the avd hw config from this new skin */
    avdInfo_getSkinHardwareIni(avd, opts->skin, opts->skindir);

    if (opts->dynamic_skin == 0) {
        opts->dynamic_skin = avdInfo_shouldUseDynamicSkin(avd);
    }

    // Read hardware configuration, apply overrides from options.
    if (avdInfo_initHwConfig(avd, hw) < 0) {
        derror("could not read hardware configuration ?");
        return -1;
    }

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

    if (!opts->charmap) {
        // Try to find a valid charmap name.
        char* charmap = avdInfo_getCharmapFile(avd, hw->hw_keyboard_charmap);
        if (charmap != NULL) {
            D("autoconfig: -charmap %s", charmap);
            opts->charmap = charmap;
        }
    }

    if (opts->charmap) {
        if (!path_exists(opts->charmap)) {
            derror("Charmap file does not exist: %s", opts->charmap);
            return -1;
        }
    }
#if 0
    // TODO(digit): Handle this better.
    if (opts->charmap) {
        /* We need to store the charmap name in the hardware configuration.
         * However, the charmap file itself is only used by the UI component
         * and doesn't need to be set to the emulation engine.
         */
        char charmap_name[SKIN_CHARMAP_NAME_SIZE];
        kcm_extract_charmap_name(opts->charmap, charmap_name,
                                 sizeof(charmap_name));
        reassign_string(&hw->hw_keyboard_charmap, charmap_name);
    }
#endif

    /* Determine snapstorage path. -no-snapstorage disables all snapshotting
     * support. This means you can't resume a snapshot at load, save it at
     * exit, or even load/save them dynamically at runtime with the console.
     */
    if (opts->no_snapstorage) {
        if (opts->snapshot) {
            dwarning("ignoring -snapshot option due to the use of -no-snapstorage");
            opts->snapshot = NULL;
        }

        if (opts->snapstorage) {
            dwarning("ignoring -snapstorage option due to the use of -no-snapstorage");
            opts->snapstorage = NULL;
        }
    } else {
        if (!opts->snapstorage && avdInfo_getSnapshotPresent(avd)) {
            opts->snapstorage = avdInfo_getSnapStoragePath(avd);
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }

        if (opts->snapstorage && !path_exists(opts->snapstorage)) {
            D("no image at '%s', state snapshots disabled", opts->snapstorage);
            opts->snapstorage = NULL;
        }
    }

    {
        EmuglConfig emuglConfig;

        if (!emuglConfig_init(&emuglConfig,
                              hw->hw_gpu_enabled,
                              hw->hw_gpu_mode,
                              opts->gpu,
                              0,
                              opts->no_window)) {
            derror("%s", emuglConfig.status);
            return -1;
        }
        hw->hw_gpu_enabled = emuglConfig.enabled;
        if (use_software_gpu_and_screen_too_large(hw)) {
            derror("GPU emulation is disabled.\n"
                   "Only screen size of 768 X 1280 or smaller is supported "
                   "when GPU emulation is disabled.");
            return -1;
        }
        if (hw->hw_gpu_enabled) {
            reassign_string(&hw->hw_gpu_mode, emuglConfig.backend);
        }
        D("%s", emuglConfig.status);
    }

    /* Quit emulator on condition that both, gpu and snapstorage are on. This is
     * a temporary solution preventing the emulator from crashing until GPU state
     * can be properly saved / resored in snapshot file. */
    if (hw->hw_gpu_enabled && opts->snapstorage && (!opts->no_snapshot_load ||
                                                    !opts->no_snapshot_save)) {
        derror("Snapshots and gpu are mutually exclusive at this point. Please turn one of them off, and restart the emulator.");
        return -1;
    }

    /* Deal with camera emulation */
    if (opts->camera_back) {
        /* Validate parameter. */
        if (memcmp(opts->camera_back, "webcam", 6) &&
            strcmp(opts->camera_back, "emulated") &&
            strcmp(opts->camera_back, "none")) {
            derror("Invalid value for -camera-back <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>', or 'none'\n",
                   opts->camera_back);
            return -1;
        }
        hw->hw_camera_back = ASTRDUP(opts->camera_back);
    }

    if (opts->camera_front) {
        /* Validate parameter. */
        if (memcmp(opts->camera_front, "webcam", 6) &&
            strcmp(opts->camera_front, "emulated") &&
            strcmp(opts->camera_front, "none")) {
            derror("Invalid value for -camera-front <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>', or 'none'\n",
                   opts->camera_front);
            return -1;
        }
        hw->hw_camera_front = ASTRDUP(opts->camera_front);
    }

    // If we have a valid snapshot storage path.

    if (opts->snapstorage) {
        hw->disk_snapStorage_path = ASTRDUP(opts->snapstorage);

        // -no-snapshot is equivalent to using both -no-snapshot-load
        // and -no-snapshot-save. You can still load/save snapshots dynamically
        //from the console though.
        if (opts->no_snapshot) {
            opts->no_snapshot_load = 1;
            opts->no_snapshot_save = 1;
            if (opts->snapshot) {
                dwarning("ignoring -snapshot option due to the use of -no-snapshot.");
            }
        }

        if (!opts->no_snapshot_load || !opts->no_snapshot_save) {
            if (!opts->snapshot) {
                opts->snapshot = ASTRDUP("default-boot");
                D("autoconfig: -snapshot %s", opts->snapshot);
            }
        }
    }

    if (!opts->logcat || opts->logcat[0] == 0) {
        opts->logcat = getenv("ANDROID_LOG_TAGS");
        if (opts->logcat && opts->logcat[0] == 0)
            opts->logcat = NULL;
    }

    /* XXXX: TODO: implement -shell and -logcat through qemud instead */
    if (!opts->shell_serial) {
#ifdef _WIN32
        opts->shell_serial = ASTRDUP("con:");
#else
        opts->shell_serial = ASTRDUP("stdio");
#endif
    } else {
        opts->shell = 1;
    }

    if (opts->selinux) {
        if ((strcmp(opts->selinux, "permissive") != 0)
                && (strcmp(opts->selinux, "disabled") != 0)) {
            derror("-selinux must be \"disabled\" or \"permissive\"");
            return -1;
        }
    }

    if (hw->vm_heapSize == 0) {
        /* Compute the default heap size based on the RAM size.
         * Essentially, we want to ensure the following liberal mappings:
         *
         *   96MB RAM -> 16MB heap
         *  128MB RAM -> 24MB heap
         *  256MB RAM -> 48MB heap
         */
        int  ramSize = hw->hw_ramSize;
        int  heapSize;

        if (ramSize < 100)
            heapSize = 16;
        else if (ramSize < 192)
            heapSize = 24;
        else
            heapSize = 48;

        hw->vm_heapSize = heapSize;
    }

    /* Setup screen emulation */
    if (opts->screen) {
        if (strcmp(opts->screen, "touch") &&
            strcmp(opts->screen, "multi-touch") &&
            strcmp(opts->screen, "no-touch")) {

            derror("Invalid value for -screen <mode> parameter: %s\n"
                   "Valid values are: touch, multi-touch, or no-touch\n",
                   opts->screen);
            return -1;
        }
        hw->hw_screen = ASTRDUP(opts->screen);
    }

    hw->avd_name = ASTRDUP(avdInfo_getName(avd));

    // NOTE: 1 is a special value. See function documentation.
    return 1;
}
