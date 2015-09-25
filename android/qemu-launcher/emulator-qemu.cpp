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
#include "android/filesystems/ext4_utils.h"
#include "android/kernel/kernel_utils.h"
#include "android/main-common.h"
#include "android/opengl/emugl_config.h"
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

enum ImageType {
    IMAGE_TYPE_SYSTEM = 0,
    IMAGE_TYPE_CACHE,
    IMAGE_TYPE_USER_DATA,
    IMAGE_TYPE_SD_CARD,
};

const int kMaxPartitions = 4;
const int kMaxTargetQemuParams = 16;
/*
 * A structure used to model information about a given target CPU architecture.
 * |androidArch| is the architecture name, following Android conventions.
 * |qemuArch| is the same name, following QEMU conventions, used to locate
 * the final qemu-system-<qemuArch> binary.
 * |qemuCpu| is the QEMU -cpu parameter value.
 * |ttyPrefix| is the prefix to use for TTY devices.
 * |kernelExtraArgs|, if not NULL, is an optional string appended to the kernel
 * parameters.
 * |storageDeviceType| is the QEMU storage device type.
 * |networkDeviceType| is the QEMU network device type.
 * |imagePartitionTypes| defines the order of how the image partitions are
 * listed in the command line, because the command line order determines which
 * mount point the partition is attached to.  For x86, the first partition
 * listed in command line is mounted first, i.e. to /dev/block/vda,
 * the next one to /dev/block/vdb, etc. However, for arm/mips, it's reversed;
 * the last one is mounted to /dev/block/vda. the 2nd last to /dev/block/vdb.
 * So far, we have 4(kMaxPartitions) types defined for system, cache, userdata
 * and sdcard images.
 * |qemuExtraArgs| are the qemu parameters specific to the target platform.
 * this is a NULL-terminated list of string pointers of at most
 * kMaxTargetQemuParams(16).
 */
struct TargetInfo {
    const char* androidArch;
    const char* qemuArch;
    const char* qemuCpu;
    const char* ttyPrefix;
    const char* kernelExtraArgs;
    const char* storageDeviceType;
    const char* networkDeviceType;
    const ImageType imagePartitionTypes[kMaxPartitions];
    const char* qemuExtraArgs[kMaxTargetQemuParams];
};

// The current target architecture information!
const TargetInfo kTarget = {
#ifdef TARGET_ARM64
    "arm64",
    "aarch64",
    "cortex-a57",
    "ttyAMA",
    " keep_bootcon earlyprintk=ttyAMA0",
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#endif
#ifdef TARGET_MIPS64
    "mips64",
    "mips64el",
    "MIPS64R6-generic",
    "ttyGF",
    NULL,
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#endif
#ifdef TARGET_X86
    "x86",
    "i386",
    "qemu32",
    "ttyS",
    " androidboot.hardware=ranchu",
    "virtio-blk-pci",
    "virtio-net-pci",
    {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_SD_CARD},
    {"-vga", "none", NULL},
#endif
#ifdef TARGET_X86_64
    "x86_64",
    "x86_64",
    "qemu64",
    "ttyS",
    " androidboot.hardware=ranchu",
    "virtio-blk-pci",
    "virtio-net-pci",
    {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_SD_CARD},
    {"-vga", "none", NULL},
#endif
};

String getNthParentDir(const char* path, size_t n) {
    StringVector dir = PathUtils::decompose(path);
    PathUtils::simplifyComponents(&dir);
    if (dir.size() < n + 1U) {
        return String("");
    }
    dir.resize(dir.size() - n);
    return PathUtils::recompose(dir);
}

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
    qemuProgram += kTarget.qemuArch;
#ifdef _WIN32
    qemuProgram += ".exe";
#endif
    path.append(qemuProgram);

    return PathUtils::recompose(path);
}

/* generate parameters for each partition by type.
 * Param:
 *  args - array to hold parameters for qemu
 *  argsPosition - current index in the parameter array
 *  driveIndex - a sequence number for the drive parameter
 *  hw - the hardware configuration that conatains image info.
 *  type - what type of partition parameter to generate
*/

void makePartitionCmd(const char** args, int* argsPosition, int* driveIndex,
                      AndroidHwConfig* hw, ImageType type) {
    int n   = *argsPosition;
    int idx = *driveIndex;

#if defined(TARGET_X86_64) || defined(TARGET_X86)
    /* for x86, 'if=none' is necessary for virtio blk*/
    String driveParam("if=none,");
#else
    String driveParam;
#endif
    String deviceParam;

    switch (type) {
        case IMAGE_TYPE_SYSTEM:
            driveParam += StringFormat("index=%d,id=system,file=%s",
                                      idx++,
                                      hw->disk_systemPartition_initPath);
            deviceParam = StringFormat("%s,drive=system",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_CACHE:
            driveParam += StringFormat("index=%d,id=cache,file=%s",
                                      idx++,
                                      hw->disk_cachePartition_path);
            deviceParam = StringFormat("%s,drive=cache",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_USER_DATA:
            driveParam += StringFormat("index=%d,id=userdata,file=%s",
                                      idx++,
                                      hw->disk_dataPartition_path);
            deviceParam = StringFormat("%s,drive=userdata",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_SD_CARD:
            if (hw->hw_sdCard_path != NULL && strcmp(hw->hw_sdCard_path, "")) {
               driveParam += StringFormat("index=%d,id=sdcard,file=%s",
                                         idx++, hw->hw_sdCard_path);
               deviceParam = StringFormat("%s,drive=sdcard",
                                          kTarget.storageDeviceType);
            } else {
                /* no sdcard is defined */
                return;
            }
            break;
        default:
            dwarning("Unknown Image type %d\n", type);
            return;
    }
    args[n++] = "-drive";
    args[n++] = ASTRDUP(driveParam.c_str());
    args[n++] = "-device";
    args[n++] = ASTRDUP(deviceParam.c_str());
    /* update the index */
    *argsPosition = n;
    *driveIndex = idx;
}

void emulator_help( void ) {
    STRALLOC_DEFINE(out);
    android_help_main(out);
    printf("%.*s", out->n, out->s);
    stralloc_reset(out);
    exit(1);
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

    String qemuExecutable = getQemuExecutablePath(argv[0]);
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
        reassign_string(&android_hw->hw_cpu_arch, kTarget.androidArch);
    }

    handleCommonEmulatorOptions(opts, hw, avd);

    {
        EmuglConfig emuglConfig;

        if (!emuglConfig_init(&emuglConfig,
                              hw->hw_gpu_enabled,
                              hw->hw_gpu_mode,
                              opts->gpu,
                              0,
                              opts->no_window)) {
            derror("%s", emuglConfig.status);
            exit(1);
        }
        hw->hw_gpu_enabled = emuglConfig.enabled;
        if (hw->hw_gpu_enabled) {
            reassign_string(&hw->hw_gpu_mode, emuglConfig.backend);
        }
        D("%s", emuglConfig.status);
    }

    hw->avd_name = ASTRDUP(avdInfo_getName(avd));

    D("QEMU EXECUTABLE=%s\n", qemuExecutable.c_str());

    // Create userdata file from init version if needed.
    if (!path_exists(hw->disk_dataPartition_path)) {
        if (!path_exists(hw->disk_dataPartition_initPath)) {
            derror("Missing initial data partition file: %s",
                   hw->disk_dataPartition_initPath);
            exit(1);
        }
        D("Creating: %s\n", hw->disk_dataPartition_path);

        if (path_copy_file(hw->disk_dataPartition_path,
                           hw->disk_dataPartition_initPath) < 0) {
            derror("Could not create %s: %s", hw->disk_dataPartition_path,
                   strerror(errno));
            exit(1);
        }
    }

    // Create cache partition image if it doesn't exist already.
    if (!path_exists(hw->disk_cachePartition_path)) {
        D("Creating empty ext4 cache partition: %s",
          hw->disk_cachePartition_path);
        int ret = android_createEmptyExt4Image(
                hw->disk_cachePartition_path,
                hw->disk_cachePartition_size,
                "cache");
        if (ret < 0) {
            derror("Could not create %s: %s", hw->disk_cachePartition_path,
                   strerror(-ret));
            exit(1);
        }
    }

    // Now build the QEMU parameters.
    const char* args[128];
    int n = 0;

    args[n++] = qemuExecutable.c_str();

#if defined(TARGET_X86_64) || defined(TARGET_X86)
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    const bool accel_ok = handleCpuAcceleration(opts, avd, &accel_mode, accel_status);

    if (accel_mode == ACCEL_OFF) {  // 'accel off' is specified'
        args[n++] = "-cpu";
        args[n++] = kTarget.qemuCpu;
    } else if (accel_mode == ACCEL_ON) {  // 'accel on' is specified'
        if (!accel_ok) {
            derror("CPU acceleration is not supported on this machine!");
            derror("Reason: %s", accel_status);
            exit(1);
        }
        args[n++] = ASTRDUP(kEnableAccelerator);
    } else {  // ACCEL_AUTO
        if (accel_ok) {
            args[n++] = ASTRDUP(kEnableAccelerator);
        } else {
            args[n++] = "-cpu";
            args[n++] = kTarget.qemuCpu;
        }
    }

    AFREE(accel_status);
#else   // !TARGET_X86_64 && !TARGET_X86
    args[n++] = "-cpu";
    args[n++] = kTarget.qemuCpu;
    args[n++] = "-machine";
    args[n++] = "type=ranchu";
#endif  // !TARGET_X86_64 && !TARGET_X86
    // Memory size
    args[n++] = "-m";
    String memorySize = StringFormat("%ld", hw->hw_ramSize);
    args[n++] = memorySize.c_str();

    // Command-line
    args[n++] = "-append";

    String kernelCommandLine = StringFormat("console=%s0,38400",
                                            kTarget.ttyPrefix);
    if (kTarget.kernelExtraArgs) {
        kernelCommandLine += kTarget.kernelExtraArgs;
    }
    if (opts->selinux) {
        kernelCommandLine += StringFormat(
                " androidboot.selinux=%s", opts->selinux);
    }

    kernelCommandLine += " qemu=1";

    if (hw->hw_gpu_enabled) {
        kernelCommandLine += " qemu.gles=1";
    } else {
        kernelCommandLine += " qemu.gles=0";
    }

    args[n++] = kernelCommandLine.c_str();

    // Support for changing default lcd-density
    String lcd_density;
    if (hw->hw_lcd_density) {
        args[n++] = "-lcd-density";
        lcd_density = StringFormat("%d", hw->hw_lcd_density);
        args[n++] = lcd_density.c_str();
    }

    args[n++] = "-serial";
    args[n++] = "mon:stdio";

    // Kernel image
    args[n++] = "-kernel";
    args[n++] = hw->kernel_path;

    // Ramdisk
    args[n++] = "-initrd";
    args[n++] = hw->disk_ramdisk_path;

    /*
     * add partition parameters with the seqeuence
     * pre-defined in targetInfo.imagePartitionTypes
     */
    int s;
    int drvIndex = 0;
    for (s = 0; s < kMaxPartitions; s++) {
        makePartitionCmd(args, &n, &drvIndex, hw, kTarget.imagePartitionTypes[s]);
    }

    // Network
    args[n++] = "-netdev";
    args[n++] = "user,id=mynet";
    args[n++] = "-device";
    String netDevice =
            StringFormat("%s,netdev=mynet", kTarget.networkDeviceType);
    args[n++] = netDevice.c_str();
    args[n++] = "-show-cursor";

    // Graphics
    if (opts->no_window) {
        args[n++] = "-nographic";
    }

    // Data directory (for keymaps and PC Bios).
    args[n++] = "-L";
    String dataDir = getNthParentDir(qemuExecutable.c_str(), 3U);
    if (dataDir.empty()) {
        dataDir = "lib/pc-bios";
    } else {
        dataDir += "/lib/pc-bios";
    }
    args[n++] = dataDir.c_str();

    /* append extra qemu parameters if any */
    for (int idx = 0; kTarget.qemuExtraArgs[idx] != NULL; idx++) {
        args[n++] = kTarget.qemuExtraArgs[idx];
    }

    /* append the options after -qemu */
    for (int i = 0; i < argc; ++i) {
        args[n++] = argv[i];
    }

    args[n] = NULL;

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
