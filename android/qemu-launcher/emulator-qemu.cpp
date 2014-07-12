// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// This source file implements emulator-arm64 and emulator64-arm64
// which are used to launch QEMU binaries located under
// $PROGRAM_DIR/qemu/<host>/qemu-system-aarch64<exe>

#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Limits.h"
#include "android/base/Log.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"

#include "android/cmdline-option.h"
#include "android/globals.h"
#include "android/help.h"
#include "android/kernel/kernel_utils.h"
#include "android/main-common.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/stralloc.h"
#include "android/utils/win32_cmdline_quote.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define  STRINGIFY(x)   _STRINGIFY(x)
#define  _STRINGIFY(x)  #x

#ifdef ANDROID_SDK_TOOLS_REVISION
#  define  VERSION_STRING  STRINGIFY(ANDROID_SDK_TOOLS_REVISION)".0"
#else
#  define  VERSION_STRING  "standalone"
#endif

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

/* The execv() definition in older mingw is slightly bogus.
 * It takes a second argument of type 'const char* const*'
 * while POSIX mandates char** instead.
 *
 * To avoid compiler warnings, define the safe_execv macro
 * to perform an explicit cast with mingw.
 */
#if defined(_WIN32) && !ANDROID_GCC_PREREQ(4,4)
#  define safe_execv(_filepath,_argv)  execv((_filepath),(const char* const*)(_argv))
#else
#  define safe_execv(_filepath,_argv)  execv((_filepath),(_argv))
#endif

using namespace android::base;

namespace {

// The host CPU architecture.
#ifdef __i386__
const char kHostArch[] = "x86";
#elif defined(__x86_64__)
const char kHostArch[] = "x86_64";
#else
#error "Your host CPU is not supported!"
#endif

// The host operating system name.
#ifdef __linux__
static const char kHostOs[] = "linux";
#elif defined(__APPLE__)
static const char kHostOs[] = "darwin";
#elif defined(_WIN32)
static const char kHostOs[] = "windows";
#endif

// The target CPU architecture.
const char kTargetArch[] = "aarch64";

// Return the path of the QEMU executable
String getQemuExecutablePath(const char* programPath) {
    StringVector path = PathUtils::decompose(programPath);
    if (path.size() < 1) {
        return String();
    }
    // Remove program from path.
    path.resize(path.size() - 1U);

    // Add sub-directories.
    path.append(String("qemu"));

    String host = kHostOs;
    host += "-";
    host += kHostArch;
    path.append(host);

    String qemuProgram = "qemu-system-";
    qemuProgram += kTargetArch;
#ifdef _WIN32
    qemuProgram += ".exe";
#endif
    path.append(qemuProgram);

    return PathUtils::recompose(path);
}

void emulator_help( void ) {
    STRALLOC_DEFINE(out);
    android_help_main(out);
    printf("%.*s", out->n, out->s);
    stralloc_reset(out);
    exit(1);
}

/* TODO: Put in shared source file */
char* _getFullFilePath(const char* rootPath, const char* fileName) {
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

uint64_t _adjustPartitionSize(const char*  description,
                              uint64_t     imageBytes,
                              uint64_t     defaultBytes,
                              int          inAndroidBuild ) {
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

bool android_op_wipe_data;

}  // namespace

extern "C" int main(int argc, char **argv, char **envp) {
    if (argc < 1) {
        fprintf(stderr, "Invalid invokation (no program path)\n");
        return 1;
    }

    AndroidOptions opts[1];

    if (android_parse_options(&argc, &argv, opts) < 0) {
        return 1;
    }

    // TODO(digit): This code is very similar to the one in main.c,
    // refactor everything so that it fits into a single shared source
    // file, if possible, with the least amount of dependencies.

    while (argc-- > 1) {
        const char* opt = (++argv)[0];

        if(!strcmp(opt, "-qemu")) {
            argc--;
            argv++;
            break;
        }

        if (!strcmp(opt, "-help")) {
            emulator_help();
        }

        if (!strncmp(opt, "-help-",6)) {
            STRALLOC_DEFINE(out);
            opt += 6;

            if (!strcmp(opt, "all")) {
                android_help_all(out);
            }
            else if (android_help_for_option(opt, out) == 0) {
                /* ok */
            }
            else if (android_help_for_topic(opt, out) == 0) {
                /* ok */
            }
            if (out->n > 0) {
                printf("\n%.*s", out->n, out->s);
                exit(0);
            }

            fprintf(stderr, "unknown option: -help-%s\n", opt);
            fprintf(stderr, "please use -help for a list of valid topics\n");
            exit(1);
        }

        if (opt[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", opt);
            fprintf(stderr, "please use -help for a list of valid options\n");
            exit(1);
        }

        fprintf(stderr, "invalid command-line parameter: %s.\n", opt);
        fprintf(stderr, "Hint: use '@foo' to launch a virtual device named 'foo'.\n");
        fprintf(stderr, "please use -help for more information\n");
        exit(1);
    }

    if (opts->version) {
        printf("Android emulator version %s\n"
               "Copyright (C) 2006-2011 The Android Open Source Project and many others.\n"
               "This program is a derivative of the QEMU CPU emulator (www.qemu.org).\n\n",
#if defined ANDROID_BUILD_ID
               VERSION_STRING " (build_id " STRINGIFY(ANDROID_BUILD_ID) ")" );
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

        exit(0);
    }

    sanitizeOptions(opts);

    // Ignore snapshot storage here.

    int inAndroidBuild = 0;
    AvdInfo* avd = createAVD(opts, &inAndroidBuild);

    // Ignore skin options here.

    // Read hardware configuration, apply overrides from options.
    AndroidHwConfig* hw = android_hw;
    if (avdInfo_initHwConfig(avd, hw) < 0) {
        derror("could not read hardware configuration ?");
        exit(1);
    }

    /* Update CPU architecture for HW configs created from build dir. */
    if (inAndroidBuild) {
#if defined(TARGET_ARM)
        reassign_string(&android_hw->hw_cpu_arch, "arm");
#elif defined(TARGET_I386)
        reassign_string(&android_hw->hw_cpu_arch, "x86");
#elif defined(TARGET_MIPS)
        reassign_string(&android_hw->hw_cpu_arch, "mips");
#elif defined(TARGET_ARM64)
        reassign_string(&android_hw->hw_cpu_arch, "arm64");
#endif
    }

    /* generate arguments for the underlying qemu main() */
    {
        char*  kernelFile    = opts->kernel;

        if (kernelFile == NULL) {
            kernelFile = avdInfo_getKernelPath(avd);
            if (kernelFile == NULL) {
                derror( "This AVD's configuration is missing a kernel file!!" );
                exit(2);
            }
            D("autoconfig: -kernel %s", kernelFile);
        }
        if (!path_exists(kernelFile)) {
            derror( "Invalid or missing kernel image file: %s", kernelFile );
            exit(2);
        }

        hw->kernel_path = kernelFile;
    }

    KernelType kernelType = KERNEL_TYPE_LEGACY;
    if (!android_pathProbeKernelType(hw->kernel_path, &kernelType)) {
        D("WARNING: Could not determine kernel device naming scheme. Assuming legacy\n"
            "If this AVD doesn't boot, and uses a recent kernel (3.10 or above) try setting\n"
            "'kernel.newDeviceNaming' to 'yes' in its configuration.\n");
    }

    // Auto-detect kernel device naming scheme if needed.
    if (androidHwConfig_getKernelDeviceNaming(hw) < 0) {
        const char* newDeviceNaming = "no";
        if (kernelType == KERNEL_TYPE_3_10_OR_ABOVE) {
            D("Auto-detect: Kernel image requires new device naming scheme.");
            newDeviceNaming = "yes";
        } else {
            D("Auto-detect: Kernel image requires legacy device naming scheme.");
        }
        AFREE(hw->kernel_newDeviceNaming);
        hw->kernel_newDeviceNaming = ASTRDUP(newDeviceNaming);
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
        AFREE(hw->kernel_supportsYaffs2);
        hw->kernel_supportsYaffs2 = ASTRDUP(newYaffs2Support);
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
            exit(1);
        }
        if (sizeMB < minSizeMB || sizeMB > maxSizeMB) {
            derror( "partition-size (%d) must be between %dMB and %dMB",
                    sizeMB, minSizeMB, maxSizeMB );
            exit(1);
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
            exit(1);
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
            exit(1);
        }

        hw->disk_systemPartition_size =
            _adjustPartitionSize("system", systemBytes, defaultPartitionSize,
                                 avdInfo_inAndroidBuild(avd));
    }

    /** DATA PARTITION **/

    if (opts->datadir) {
        if (!path_exists(opts->datadir)) {
            derror("Invalid -datadir directory: %s", opts->datadir);
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
                    exit (1);
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
                exit(1);
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
            exit(1);
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
                fprintf(stderr, "### WARNING: SD Card files must be at least 9MB, ignoring '%s'\n", opts->sdcard);
            } else {
                hw->hw_sdCard_path = ASTRDUP(opts->sdcard);
            }
        } else {
            dwarning("no SD Card image at '%s'", opts->sdcard);
        }
    }

    if (opts->memory) {
        char*  end;
        long   ramSize = strtol(opts->memory, &end, 0);
        if (ramSize < 0 || *end != 0) {
            derror( "-memory must be followed by a positive integer" );
            exit(1);
        }
        if (ramSize < 32 || ramSize > 4096) {
            derror( "physical memory size must be between 32 and 4096 MB" );
            exit(1);
        }
        hw->hw_ramSize = ramSize;
    } else {
        int ramSize = hw->hw_ramSize;
        if (ramSize <= 0) {
#if 1
            // For ARM64, use 1GB by default.
            ramSize = 1024 * 1024ULL;
#else
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
#endif
        hw->hw_ramSize = ramSize;
    }

    D("Physical RAM size: %dMB\n", hw->hw_ramSize);

    if (opts->gpu) {
        const char* gpu = opts->gpu;
        if (!strcmp(gpu,"on") || !strcmp(gpu,"enable")) {
            hw->hw_gpu_enabled = 1;
        } else if (!strcmp(gpu,"off") || !strcmp(gpu,"disable")) {
            hw->hw_gpu_enabled = 0;
        } else if (!strcmp(gpu,"auto")) {
            /* Nothing to do */
        } else {
            derror("Invalid value for -gpu <mode> parameter: %s\n", gpu);
            derror("Valid values are: on, off or auto\n");
            exit(1);
        }
    }

    hw->avd_name = ASTRDUP(avdInfo_getName(avd));

    String qemuExecutable = getQemuExecutablePath(argv[0]);
    D("QEMU EXECUTABLE=%s\n", qemuExecutable.c_str());

    // Now build the QEMU parameters.
    const char* args[128];
    int n = 0;

    args[n++] = qemuExecutable.c_str();

    args[n++] = "-cpu";
    args[n++] = "cortex-a57";
    args[n++] = "-machine";
    args[n++] = "type=ranchu";

    // Memory size
    args[n++] = "-m";
    String memorySize = StringFormat("%ld", hw->hw_ramSize);
    args[n++] = memorySize.c_str();

    // Command-line
    args[n++] = "-append";
    String kernelCommandLine =
            "console=ttyAMA0,38400 keep_bootcon earlyprintk=ttyAMA0";
    args[n++] = kernelCommandLine.c_str();

    args[n++] = "-serial";
    args[n++] = "mon:stdio";

    // Kernel image
    args[n++] = "-kernel";
    args[n++] = hw->kernel_path;

    // Ramdisk
    args[n++] = "-initrd";
    args[n++] = hw->disk_ramdisk_path;

    // Data partition.
    args[n++] = "-drive";
    String userDataParam =
            StringFormat("index=2,id=userdata,file=%s",
                         hw->disk_dataPartition_path);
    args[n++] = userDataParam.c_str();
    args[n++] = "-device";
    args[n++] = "virtio-blk-device,drive=userdata";

    // Cache partition.
    args[n++] = "-drive";
    String cacheParam =
            StringFormat("index=1,id=cache,file=%s",
                         hw->disk_cachePartition_path);
    args[n++] = cacheParam.c_str();
    args[n++] = "-device";
    args[n++] = "virtio-blk-device,drive=cache";

    // System partition.
    args[n++] = "-drive";
    String systemParam =
            StringFormat("index=0,id=system,file=%s",
                         hw->disk_systemPartition_initPath);
    args[n++] = systemParam.c_str();
    args[n++] = "-device";
    args[n++] = "virtio-blk-device,drive=system";

    // Network
    args[n++] = "-netdev";
    args[n++] = "user,id=mynet";
    args[n++] = "-device";
    args[n++] = "virtio-net-device,netdev=mynet";
    args[n++] = "-show-cursor";

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

    if (!path_exists(qemuExecutable.c_str())) {
        fprintf(stderr, "Missing QEMU executable: %s\n",
                qemuExecutable.c_str());
        return 1;
    }

    // Now launch executable.
#ifdef _WIN32
    // Take care of quoting all parameters before sending them to execv().
    // See the "Eveyone quotes command line arguments the wrong way" on
    // MSDN.
    int i;
    for (i = 0; i < n; ++i) {
        // Technically, this leaks the quoted strings, but we don't care
        // since this process will terminate after the execv() anyway.
        args[i] = win32_cmdline_quote(args[i]);
        D("Quoted param: [%s]\n", args[i]);
    }
#endif

    safe_execv(qemuExecutable.c_str(), (char* const*)args);

    fprintf(stderr,
            "Could not launch QEMU [%s]: %s\n",
            qemuExecutable.c_str(),
            strerror(errno));

    return errno;
}
}
}
}
