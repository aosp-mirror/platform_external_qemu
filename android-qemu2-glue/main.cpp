// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"

#include "android/android.h"
#include "android/avd/hw-config.h"
#include "android/boot-properties.h"
#include "android/cmdline-option.h"
#include "android/constants.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/ConfigDirs.h"
#include "android/error-messages.h"
#include "android/featurecontrol/feature_control.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/filesystems/ext4_resize.h"
#include "android/filesystems/ext4_utils.h"
#include "android/globals.h"
#include "android/help.h"
#include "android/kernel/kernel_utils.h"
#include "android/main-common.h"
#include "android/main-common-ui.h"
#include "android/main-kernel-parameters.h"
#include "android/opengles.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/process_setup.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "android/utils/lineinput.h"
#include "android/utils/property_file.h"
#include "android/utils/filelock.h"
#include "android/utils/stralloc.h"
#include "android/utils/string.h"
#include "android/utils/tempfile.h"
#include "android/utils/win32_cmdline_quote.h"

#include "android/skin/winsys.h"
#include "android/skin/qt/init-qt.h"

#include "config-target.h"

extern "C" {
#include "android/skin/charmap.h"
}

#include "android/ui-emu-agent.h"
#include "android-qemu2-glue/emulation/serial_line.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/qemu-control-impl.h"

#ifdef TARGET_AARCH64
#define TARGET_ARM64
#endif
#ifdef TARGET_I386
#define TARGET_X86
#endif

#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "android/version.h"
#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

int android_base_port;
int android_serial_number_port;

extern bool android_op_wipe_data;
extern bool android_op_writable_system;

using namespace android::base;
using android::base::System;

namespace {

enum ImageType {
    IMAGE_TYPE_SYSTEM = 0,
    IMAGE_TYPE_CACHE,
    IMAGE_TYPE_USER_DATA,
    IMAGE_TYPE_SD_CARD,
    IMAGE_TYPE_ENCRYPTION_KEY,
};

const int kMaxPartitions = 5;
const int kMaxTargetQemuParams = 16;

/*
 * A structure used to model information about a given target CPU architecture.
 * |androidArch| is the architecture name, following Android conventions.
 * |qemuArch| is the same name, following QEMU conventions, used to locate
 * the final qemu-system-<qemuArch> binary.
 * |qemuCpu| is the QEMU -cpu parameter value.
 * |ttyPrefix| is the prefix to use for TTY devices.
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
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#elif defined(TARGET_ARM)
    "arm",
    "arm",
    "cortex-a15",
    "ttyAMA",
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#elif defined(TARGET_MIPS64)
    "mips64",
    "mips64el",
    "MIPS64R6-generic",
    "ttyGF",
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#elif defined(TARGET_MIPS)
    "mips",
    "mipsel",
    "74Kf",
    "ttyGF",
    "virtio-blk-device",
    "virtio-net-device",
    {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
    {NULL},
#elif defined(TARGET_X86_64)
    "x86_64",
    "x86_64",
    "android64",
    "ttyS",
    "virtio-blk-pci",
    "virtio-net-pci",
    {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_SD_CARD},
    {"-vga", "none", NULL},
#elif defined(TARGET_I386)  // Both i386 and x86_64 targets define this macro
    "x86",
    "i386",
    "android32",
    "ttyS",
    "virtio-blk-pci",
    "virtio-net-pci",
    {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA, IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_SD_CARD},
    {"-vga", "none", NULL},
#else
    #error No target platform is defined
#endif
};

static std::string getNthParentDir(const char* path, size_t n) {
    auto dirs = PathUtils::decompose(path);
    PathUtils::simplifyComponents(&dirs);
    if (dirs.size() < n + 1U) {
        return std::string("");
    }
    dirs.resize(dirs.size() - n);
    return PathUtils::recompose(dirs);
}

/* generate parameters for each partition by type.
 * Param:
 *  args - array to hold parameters for qemu
 *  argsPosition - current index in the parameter array
 *  driveIndex - a sequence number for the drive parameter
 *  hw - the hardware configuration that conatains image info.
 *  type - what type of partition parameter to generate
*/

static void makePartitionCmd(const char** args, int* argsPosition, int* driveIndex,
                             AndroidHwConfig* hw, ImageType type, bool writable,
                             int apiLevel, const char* avdContentPath) {
    int n   = *argsPosition;
    int idx = *driveIndex;

#if defined(TARGET_X86_64) || defined(TARGET_I386)
    /* for x86, 'if=none' is necessary for virtio blk*/
    std::string driveParam("if=none,");
#else
    std::string driveParam;
#endif
    // Disable extra qcow2 checks as we're on its stable version.
    // Disable cache flushes as well, as Android issues way too many flush
    // commands for nothing.
    driveParam += "overlap-check=none,cache=unsafe,";

    std::string deviceParam;
    StringView filePath;
    switch (type) {
        case IMAGE_TYPE_SYSTEM:
            filePath = avdContentPath;
            driveParam += StringFormat("index=%d,id=system,file=%s"
                                       PATH_SEP "system.img.qcow2",
                                        idx++, filePath);
            // API 15 and under images need a read+write
            // system image.
            if (apiLevel > 15) {
                // API > 15 uses read-only system partition.
                // You can override this explicitly
                // by passing -writable-system to emulator.
                if (!writable)
                    driveParam += ",read-only";
            }
            deviceParam = StringFormat("%s,drive=system",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_CACHE:
            filePath = hw->disk_cachePartition_path;
            driveParam += StringFormat("index=%d,id=cache,file=%s.qcow2",
                                      idx++,
                                      filePath);
            deviceParam = StringFormat("%s,drive=cache",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_USER_DATA:
            filePath = hw->disk_dataPartition_path;
            driveParam += StringFormat("index=%d,id=userdata,file=%s.qcow2",
                                      idx++,
                                      filePath);
            deviceParam = StringFormat("%s,drive=userdata",
                                       kTarget.storageDeviceType);
            break;
        case IMAGE_TYPE_SD_CARD:
            if (hw->hw_sdCard_path != NULL && strcmp(hw->hw_sdCard_path, "")) {
                filePath = hw->hw_sdCard_path;
                driveParam += StringFormat("index=%d,id=sdcard,file=%s.qcow2",
                                          idx++, filePath);
                deviceParam = StringFormat("%s,drive=sdcard",
                                           kTarget.storageDeviceType);
            } else {
                /* no sdcard is defined */
                return;
            }
            break;
        case IMAGE_TYPE_ENCRYPTION_KEY:
            if (android::featurecontrol::isEnabled(android::featurecontrol::EncryptUserData) &&
                hw->disk_encryptionKeyPartition_path != NULL && strcmp(hw->disk_encryptionKeyPartition_path, "")) {
                filePath = hw->disk_encryptionKeyPartition_path;
                driveParam += StringFormat("index=%d,id=encrypt,file=%s.qcow2",
                                           idx++, filePath);
                deviceParam = StringFormat("%s,drive=encrypt",
                                           kTarget.storageDeviceType);
            } else {
                /* no encryption partition is defined */
                return;
            }
            break;
        default:
            dwarning("Unknown Image type %d\n", type);
            return;
    }

    // Default qcow2's L2 cache size is up to 8GB. Let's increase it for
    // larger images.
    System::FileSize diskSize;
    if (System::get()->pathFileSize(filePath, &diskSize)) {
        // L2 cache size should be "disk_size_GB / 131072" as per QEMU docs
        // with a default of 1MB. Round it up just in case.
        const int l2CacheSize =
                std::max<int>(
                    (diskSize + (1024 * 1024 * 1024 - 1)) / (1024 * 1024 * 1024) * 131072,
                    1024 * 1024);
        driveParam += StringFormat(",l2-cache-size=%d", l2CacheSize);
    }

    // Move the disk operations into the dedicated 'disk thread', and
    // enable modern notification mode for the hosts that support it (Linux).
#if defined(TARGET_X86_64) || defined(TARGET_I386)
#ifdef CONFIG_LINUX
    // eventfd is required for this, and only available on kvm.
    deviceParam += ",iothread=disk-iothread";
#endif
    deviceParam += ",modern-pio-notify";
#endif

    args[n++] = "-drive";
    args[n++] = ASTRDUP(driveParam.c_str());
    args[n++] = "-device";
    args[n++] = ASTRDUP(deviceParam.c_str());
    /* update the index */
    *argsPosition = n;
    *driveIndex = idx;
}


}  // namespace


extern "C" int run_qemu_main(int argc, const char **argv);

static void enter_qemu_main_loop(int argc, char **argv) {
#ifndef _WIN32
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif

    D("Starting QEMU main loop");
    run_qemu_main(argc, (const char**)argv);
    D("Done with QEMU main loop");

    if (android_init_error_occurred()) {
        skin_winsys_error_dialog(android_init_error_get_message(), "Error");
    }
}

#if defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain().
#define main qt_main
#endif

static bool createInitalEncryptionKeyPartition(AndroidHwConfig* hw) {
    char* userdata_dir = path_dirname(hw->disk_dataPartition_path);
    if (!userdata_dir) {
        derror("no userdata_dir");
        return false;
    }
    hw->disk_encryptionKeyPartition_path = path_join(userdata_dir, "encryptionkey.img");
    free(userdata_dir);
    if (path_exists(hw->disk_systemPartition_initPath)) {
        char* sysimg_dir = path_dirname(hw->disk_systemPartition_initPath);
        if (!sysimg_dir) {
            derror("no sysimg_dir %s", hw->disk_systemPartition_initPath);
            return false;
        }
        char* init_encryptionkey_img_path = path_join(sysimg_dir, "encryptionkey.img");
        free(sysimg_dir);
        if (path_exists(init_encryptionkey_img_path)) {
            if (path_copy_file(hw->disk_encryptionKeyPartition_path, init_encryptionkey_img_path) >= 0) {
                free(init_encryptionkey_img_path);
                return true;
            }
        } else {
            derror("no init encryptionkey.img");
        }
        free(init_encryptionkey_img_path);
    } else {
        derror("no system partition %s", hw->disk_systemPartition_initPath);
    }
    return false;
}

extern "C" int main(int argc, char **argv) {
    process_early_setup(argc, argv);

    if (argc < 1) {
        fprintf(stderr, "Invalid invocation (no program path)\n");
        return 1;
    }

    /* The emulator always uses the first serial port for kernel messages
     * and the second one for qemud. So start at the third if we need one
     * for logcat or 'shell'
     */
    const char* args[128];
    args[0] = argv[0];
    int n = 1;  // next parameter index

    AndroidHwConfig* hw = android_hw;
    AvdInfo* avd;
    AndroidOptions opts[1];
    int exitStatus = 0;

    if (!emulator_parseCommonCommandLineOptions(&argc,
                                                &argv,
                                                kTarget.androidArch,
                                                true,  // is_qemu2
                                                opts,
                                                hw,
                                                &android_avdInfo,
                                                &exitStatus)) {
        // Special case for QEMU positional parameters.
        if (exitStatus == EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER) {
            // Copy all QEMU options to |args|, and set |n| to the number
            // of options in |args| (|argc| must be positive here).
            for (n = 1; n <= argc; ++n) {
                args[n] = argv[n - 1];
            }

            // Skip the translation of command-line options and jump
            // straight to qemu_main().
            enter_qemu_main_loop(n, (char**)args);
            return 0;
        }

        // Normal exit.
        return exitStatus;
    }

    // Update server-based hw config / feature flags.
    // Must be done after emulator_parseCommonCommandLineOptions,
    // since that calls createAVD which sets up critical info needed
    // by featurecontrol component itself.
    feature_update_from_server();

    // just because we know that we're in the new emulator as we got here
    opts->ranchu = 1;

    avd = android_avdInfo;

    if (!emulator_parseUiCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    char boot_prop_ip[128] = {};
    if (opts->shared_net_id) {
        char*  end;
        long   shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            fprintf(stderr, "option -shared-net-id must be an integer between 1 and 255\n");
            return 1;
        }
        snprintf(boot_prop_ip, sizeof(boot_prop_ip),
                 "net.shared_net_ip=10.1.2.%ld", shared_net_id);
    }
    if (boot_prop_ip[0]) {
        args[n++] = "-boot-property";
        args[n++] = boot_prop_ip;
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

    if (opts->audio && !strcmp(opts->audio, "none")) {
        args[n++] = "-no-audio";
    }

    if (opts->cpu_delay) {
        args[n++] = "-cpu-delay";
        args[n++] = opts->cpu_delay;
    }

    if (opts->dns_server) {
        args[n++] = "-dns-server";
        args[n++] = opts->dns_server;
    }

    if (opts->skip_adb_auth) {
        args[n++] = "-skip-adb-auth";
    }

    /** SNAPSHOT STORAGE HANDLING */

    /* If we have a valid snapshot storage path */

    if (opts->snapstorage) {
        // NOTE: If QEMU2_SNAPSHOT_SUPPORT is not defined, a warning has been
        //       already printed by emulator_parseCommonCommandLineOptions().
#ifdef QEMU2_SNAPSHOT_SUPPORT
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
#endif  // QEMU2_SNAPSHOT_SUPPORT
    }

    if (android::featurecontrol::isEnabled(android::featurecontrol::LogcatPipe) && opts->logcat) {
        boot_property_add_logcat_pipe(opts->logcat);
        // we have done with -logcat option.
        opts->logcat = NULL;
    }

    {
        // Always setup a single serial port, that can be connected
        // either to the 'null' chardev, or the -shell-serial one,
        // which by default will be either 'stdout' (Posix) or 'con:'
        // (Windows).
        const char* serial =
                (opts->shell || opts->logcat || opts->show_kernel)
                ? opts->shell_serial : "null";
        args[n++] = "-serial";
        args[n++] = serial;
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

    if (opts->ports) {
        args[n++] = "-android-ports";
        args[n++] = opts->ports;
    }

    if (opts->port) {
        int console_port = -1;
        int adb_port = -1;
        if (!android_parse_port_option(opts->port, &console_port, &adb_port)) {
            return 1;
        }
        std::string portsOption = StringFormat("%d,%d",
                                               console_port, adb_port);
        args[n++] = "-android-ports";
        args[n++] = strdup(portsOption.c_str());
    }

    if (opts->report_console) {
        args[n++] = "-android-report-console";
        args[n++] = opts->report_console;
    }

    if (opts->http_proxy) {
        if (!qemu_android_setup_http_proxy(opts->http_proxy)) {
            return 1;
        }
    }

    if (!opts->charmap) {
        /* Try to find a valid charmap name */
        char* charmap = avdInfo_getCharmapFile(avd, hw->hw_keyboard_charmap);
        if (charmap != NULL) {
            D("autoconfig: -charmap %s", charmap);
            opts->charmap = charmap;
        }
    }

    if (opts->charmap) {
        char charmap_name[SKIN_CHARMAP_NAME_SIZE];

        if (!path_exists(opts->charmap)) {
            derror("Charmap file does not exist: %s", opts->charmap);
            return 1;
        }
        /* We need to store the charmap name in the hardware configuration.
         * However, the charmap file itself is only used by the UI component
         * and doesn't need to be set to the emulation engine.
         */
        kcm_extract_charmap_name(opts->charmap, charmap_name,
                                 sizeof(charmap_name));
        reassign_string(&hw->hw_keyboard_charmap, charmap_name);
    }

// TODO: imement network
#if 0
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
#endif

    // Create userdata file from init version if needed.
    if (android_op_wipe_data || !path_exists(hw->disk_dataPartition_path)) {
        std::unique_ptr<char[]> initDir(avdInfo_getDataInitDirPath(avd));
        if (path_exists(initDir.get())) {
            std::string dataPath = PathUtils::join(
                    avdInfo_getContentPath(avd), "data");
            path_copy_dir(dataPath.c_str(), initDir.get());
            std::string adbKeyPath = PathUtils::join(
                    android::ConfigDirs::getUserDirectory(), "adbkey.pub");
            if (path_is_regular(adbKeyPath.c_str())
                    && path_can_read(adbKeyPath.c_str())) {
                std::string guestAdbKeyDir = PathUtils::join(
                        dataPath, "misc", "adb");
                std::string guestAdbKeyPath = PathUtils::join(
                        guestAdbKeyDir, "adb_keys");
                path_mkdir_if_needed(guestAdbKeyDir.c_str(), 0777);
                path_copy_file(
                        guestAdbKeyPath.c_str(),
                        adbKeyPath.c_str());
                android_chmod(guestAdbKeyPath.c_str(), 0777);
            } else {
                dwarning("cannot read adb public key file: %d",
                         adbKeyPath.c_str());
            }
            android_createExt4ImageFromDir(hw->disk_dataPartition_path,
                    dataPath.c_str(),
                    android_hw->disk_dataPartition_size,
                    "data");
            // TODO: remove dataPath folder
        } else if (path_exists(hw->disk_dataPartition_initPath)) {
            D("Creating: %s\n", hw->disk_dataPartition_path);

            if (path_copy_file(hw->disk_dataPartition_path,
                               hw->disk_dataPartition_initPath) < 0) {
                derror("Could not create %s: %s", hw->disk_dataPartition_path,
                       strerror(errno));
                return 1;
            }

            resizeExt4Partition(android_hw->disk_dataPartition_path,
                                android_hw->disk_dataPartition_size);
        } else {
            derror("Missing initial data partition file: %s",
                   hw->disk_dataPartition_initPath);
        }
    }
    else {
        // Resize userdata-qemu.img if the size is smaller than what config.ini
        // says.
        // This can happen as user wants a larger data partition without wiping
        // it.
        // b.android.com/196926
        System::FileSize current_data_size;
        if (System::get()->pathFileSize(hw->disk_dataPartition_path,
                                        &current_data_size)) {
            System::FileSize partition_size = static_cast<System::FileSize>(
                    android_hw->disk_dataPartition_size);
            if (android_hw->disk_dataPartition_size > 0 &&
                    current_data_size < partition_size) {
                dwarning("userdata partition is resized from %d M to %d M\n",
                         (int)(current_data_size / (1024 * 1024)),
                         (int)(partition_size / (1024 * 1024)));
                resizeExt4Partition(android_hw->disk_dataPartition_path,
                                    android_hw->disk_dataPartition_size);
            }
        }
    }

    // create encryptionkey.img file if needed
    if (android::featurecontrol::isEnabled(android::featurecontrol::EncryptUserData)) {
        if (hw->disk_encryptionKeyPartition_path == NULL) {
            if(!createInitalEncryptionKeyPartition(hw)) {
                derror("Encryption is requested but failed to create encrypt partition.");
                return 1;
            }
        }
    } else {
        dwarning("encryption is off");
    }

    bool createEmptyCacheFile = false;

    // Make sure there's a temp cache partition if there wasn't a permanent one
    if (!hw->disk_cachePartition_path ||
        strcmp(hw->disk_cachePartition_path, "") == 0) {
        str_reset(&hw->disk_cachePartition_path,
                  tempfile_path(tempfile_create()));
        createEmptyCacheFile = true;
    }

    createEmptyCacheFile |= !path_exists(hw->disk_cachePartition_path);

    if (createEmptyCacheFile) {
        D("Creating empty ext4 cache partition: %s",
          hw->disk_cachePartition_path);
        int ret = android_createEmptyExt4Image(
                hw->disk_cachePartition_path,
                hw->disk_cachePartition_size,
                "cache");
        if (ret < 0) {
            derror("Could not create %s: %s", hw->disk_cachePartition_path,
                   strerror(-ret));
            return 1;
        }
    }

    // Make sure we always use the custom Android CPU definition.
    args[n++] = "-cpu";
#if defined(TARGET_MIPS)
    args[n++] = (hw->hw_cpu_model && hw->hw_cpu_model[0]) ? hw->hw_cpu_model
                                                          : kTarget.qemuCpu;
#else
    args[n++] = kTarget.qemuCpu;
#endif

    // Set env var to "on" for Intel PMU if the feature is enabled.
    // cpu.c will then read that.
    if (android::featurecontrol::isEnabled(android::featurecontrol::IntelPerformanceMonitoringUnit)) {
        System::get()->envSet("ANDROID_EMU_FEATURE_IntelPerformanceMonitoringUnit", "on");
    }

#if defined(TARGET_X86_64) || defined(TARGET_I386)
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    const bool accel_ok = handleCpuAcceleration(opts, avd,
                                                &accel_mode, &accel_status);

    if (accel_mode == ACCEL_ON) {  // 'accel on' is specified'
        if (!accel_ok) {
            derror("CPU acceleration is not supported on this machine!");
            derror("Reason: %s", accel_status);
            return 1;
        }
        args[n++] = ASTRDUP(kEnableAccelerator);
    } else if (accel_mode == ACCEL_AUTO) {
        if (accel_ok) {
            args[n++] = ASTRDUP(kEnableAccelerator);
        }
    }
    else if (accel_mode == ACCEL_HVF) {
#if CONFIG_HVF
        args[n++] = ASTRDUP(kEnableAcceleratorHVF);
#endif
    } // else, add other special situations to enable particular
      // acceleration backends (e.g., HyperV/KVM on Windows,
      // KVM on Mac, etc.)

    AFREE(accel_status);
#else   // !TARGET_X86_64 && !TARGET_I386
    args[n++] = "-machine";
    args[n++] = "type=ranchu";
#endif  // !TARGET_X86_64 && !TARGET_I386

#if defined(TARGET_X86_64) || defined(TARGET_I386)
    // SMP Support.
    std::string ncores;
    if (hw->hw_cpu_ncore > 1) {
        args[n++] = "-smp";

#ifdef _WIN32
        if (hw->hw_cpu_ncore > 16) {
            dwarning("HAXM does not support more than 16 cores. Number of cores set to 16");
            hw->hw_cpu_ncore = 16;
        }
#endif
        ncores = StringFormat("cores=%d", hw->hw_cpu_ncore);
        args[n++] = ncores.c_str();
    }
#endif  // !TARGET_X86_64 && !TARGET_I386

    // Memory size
    args[n++] = "-m";
    std::string memorySize = StringFormat("%d", hw->hw_ramSize);
    args[n++] = memorySize.c_str();

    int apiLevel = avd ? avdInfo_getApiLevel(avd) : 1000;

    // Support for changing default lcd-density
    std::string lcd_density;
    if (hw->hw_lcd_density) {
        args[n++] = "-lcd-density";
        lcd_density = StringFormat("%d", hw->hw_lcd_density);
        args[n++] = lcd_density.c_str();
    }

    // Kernel image
    args[n++] = "-kernel";
    args[n++] = hw->kernel_path;

    // Ramdisk
    args[n++] = "-initrd";
    args[n++] = hw->disk_ramdisk_path;

    // Dedicated IOThread for all disk IO
    args[n++] = "-object";
    args[n++] = "iothread,id=disk-iothread";

    // Don't create the default CD drive and floppy disk devices - Android
    // won't appreciate it.
    args[n++] = "-nodefaults";

    /*
     * add partition parameters with the sequence
     * pre-defined in targetInfo.imagePartitionTypes
     */
    int s;
    int drvIndex = 0;
    for (s = 0; s < kMaxPartitions; s++) {
        bool writable = (kTarget.imagePartitionTypes[s] == IMAGE_TYPE_SYSTEM) ?
                    android_op_writable_system : true;
        makePartitionCmd(args, &n, &drvIndex, hw,
                         kTarget.imagePartitionTypes[s], writable, apiLevel,
                         avdInfo_getContentPath(avd));
    }

    // Network
    args[n++] = "-netdev";
    args[n++] = "user,id=mynet";
    args[n++] = "-device";
    std::string netDevice =
            StringFormat("%s,netdev=mynet", kTarget.networkDeviceType);
    args[n++] = netDevice.c_str();
 
    // add 2nd nic as eth1
    args[n++] = "-netdev";
    args[n++] = "user,id=mynet2,net=10.0.3.0/24";
    args[n++] = "-device";
    std::string netDevice2 =
            StringFormat("%s,netdev=mynet2", kTarget.networkDeviceType);
    args[n++] = netDevice2.c_str();

    args[n++] = "-show-cursor";

    // TODO: the following *should* re-enable -tcpdump in QEMU2 when we have
    // rebased to at least QEMU 2.5 - the standard -tcpdump flag
    // See http://wiki.qemu.org/ChangeLog/2.5#Network_2 and
    // http://wiki.qemu.org/download/qemu-doc.html#index-_002dobject
//    std::string tcpdumpArg;
//    if (opts->tcpdump) {
//        args[n++] = "-object";
//        tcpdumpArg = StringFormat("filter-dump,id=mytcpdump,netdev=mynet,file=%s",
//                                  opts->tcpdump);
//        args[n++] = tcpdumpArg.c_str();
//    }

    if (opts->tcpdump) {
        dwarning("The -tcpdump flag is not supported in QEMU2 yet and will "
                 "be ignored.");
    }

    // Graphics
    if (opts->no_window) {
        args[n++] = "-nographic";
        // also disable the qemu monitor which will otherwise grab stdio
        args[n++] = "-monitor";
        args[n++] = "none";
    }

    // Data directory (for keymaps and PC Bios).
    args[n++] = "-L";
    std::string dataDir = getNthParentDir(args[0], 3U);
    if (dataDir.empty()) {
        dataDir = "lib/pc-bios";
    } else {
        dataDir += "/lib/pc-bios";
    }
    args[n++] = dataDir.c_str();

    // Audio enable hda by default for x86 and x64 platforms
#if defined(TARGET_X86_64) || defined(TARGET_I386)
    args[n++] = "-soundhw";
    args[n++] = "hda";
#endif

    /* append extra qemu parameters if any */
    for (int idx = 0; kTarget.qemuExtraArgs[idx] != NULL; idx++) {
        args[n++] = kTarget.qemuExtraArgs[idx];
    }

    // Setup GPU acceleration. This needs to go along with user interface
    // initialization, because we need the selected backend from Qt settings.
    const UiEmuAgent uiEmuAgent = {
            gQAndroidBatteryAgent,
            gQAndroidCellularAgent,
            gQAndroidClipboardAgent,
            gQAndroidEmulatorWindowAgent,
            gQAndroidFingerAgent,
            gQAndroidLocationAgent,
            gQAndroidHttpProxyAgent,
            gQAndroidSensorsAgent,
            gQAndroidTelephonyAgent,
            gQAndroidUserEventAgent,
            gQCarDataAgent,
            nullptr  // For now there's no uses of SettingsAgent, so we
                     // don't set it.
    };

    {
        qemu2_android_serialline_init();

        /* Setup SDL UI just before calling the code */
        android::base::Thread::maskAllSignals();

        skin_winsys_init_args(argc, argv);
        if (!emulator_initUserInterface(opts, &uiEmuAgent)) {
            return 1;
        }

        // Use advancedFeatures to override renderer if the user has selected
        // in UI that the preferred renderer is "autoselected".
        WinsysPreferredGlesBackend uiPreferredGlesBackend =
            skin_winsys_get_preferred_gles_backend();

        // Feature flags-related last-microsecond renderer changes
        {
            // Should enable OpenGL ES 3.x?
            if (!android::featurecontrol::isEnabled(android::featurecontrol::GLESDynamicVersion) &&
                    skin_winsys_get_preferred_gles_apilevel() == WINSYS_GLESAPILEVEL_PREFERENCE_MAX) {
                android::featurecontrol::setEnabledOverride(
                        android::featurecontrol::GLESDynamicVersion, true);
            }

            if (android::featurecontrol::isEnabled(android::featurecontrol::ForceANGLE)) {
                uiPreferredGlesBackend =
                    skin_winsys_override_glesbackend_if_auto(WINSYS_GLESBACKEND_PREFERENCE_ANGLE);
            }

            if (android::featurecontrol::isEnabled(android::featurecontrol::ForceSwiftshader)) {
                uiPreferredGlesBackend =
                    skin_winsys_override_glesbackend_if_auto(WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER);
            }

            // Features to disable or enable depending on rendering backend
            // and gpu make/model/version
            /* Disable the GLAsyncSwap for ANGLE so far */
            bool shouldDisableAsyncSwap = false;
            shouldDisableAsyncSwap |= (opts->gpu && !strncmp(opts->gpu, "angle", 5));
            shouldDisableAsyncSwap |= async_query_host_gpu_SyncBlacklisted();

            if (shouldDisableAsyncSwap &&
                    android::featurecontrol::isEnabled(android::featurecontrol::GLAsyncSwap)) {
                android::featurecontrol::setEnabledOverride(
                        android::featurecontrol::GLAsyncSwap, false);
            }

        }

        RendererConfig rendererConfig;
        configAndStartRenderer(
            avd, opts, hw, uiPreferredGlesBackend,
            &rendererConfig);

        char* kernel_parameters = emulator_getKernelParameters(
                opts, kTarget.androidArch, apiLevel, kTarget.ttyPrefix,
                hw->kernel_parameters,
                rendererConfig.glesMode, rendererConfig.bootPropOpenglesVersion,
                rendererConfig.glFramebufferSizeBytes,
                true  /* isQemu2 */);

        if (!kernel_parameters) {
            return 1;
        }

        /* append the kernel parameters after -qemu */
        std::string append_arg(kernel_parameters);
        free(kernel_parameters);
        for (int i = 0; i < argc; ++i) {
            if (!strcmp(argv[i], "-append")) {
                if (++i < argc) {
                    android::base::StringAppendFormat(&append_arg, " %s", argv[i]);
                }
            } else {
                args[n++] = argv[i];
            }
        }
        args[n++] = "-append";
        args[n++] = ASTRDUP(append_arg.c_str());
    }

    /* Generate a hardware-qemu.ini for this AVD. The real hardware
     * configuration is ususally stored in several files, e.g. the AVD's
     * config.ini plus the skin-specific hardware.ini.
     *
     * The new file will group all definitions and will be used to
     * launch the core with the -android-hw <file> option.
     */
    {
        const char* coreHwIniPath = avdInfo_getCoreHwIniPath(avd);
        const auto hwIni = android::base::makeCustomScopedPtr(
                         iniFile_newEmpty(NULL), iniFile_free);
        androidHwConfig_write(hw, hwIni.get());

        if (filelock_create(coreHwIniPath) == NULL) {
            // The AVD is already in use
            derror("There's another emulator instance running with "
                   "the current AVD '%s'. Exiting...\n", avdInfo_getName(avd));
            return 1;
        }

        /* While saving HW config, ignore valueless entries. This will not break
         * anything, but will significantly simplify comparing the current HW
         * config with the one that has been associated with a snapshot (in case
         * VM starts from a snapshot for this instance of emulator). */
        if (iniFile_saveToFileClean(hwIni.get(), coreHwIniPath) < 0) {
            derror("Could not write hardware.ini to %s: %s", coreHwIniPath, strerror(errno));
            return 2;
        }
        args[n++] = "-android-hw";
        args[n++] = strdup(coreHwIniPath);

        crashhandler_copy_attachment(CRASH_AVD_HARDWARE_INFO, coreHwIniPath);

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

    args[n] = NULL;
    // Check if we had enough slots in |args|.
    assert(n < (int)(sizeof(args)/sizeof(args[0])));

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

    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop, n, (char**)args);
    skin_winsys_enter_main_loop(opts->no_window);

    emulator_finiUserInterface();

    process_late_teardown();
    return 0;
}
