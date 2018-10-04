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

#include "android/android.h"
#include "android/avd/hw-config.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/ProcessControl.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/boot-properties.h"
#include "android/camera/camera-virtualscene.h"
#include "android/cmdline-option.h"
#include "android/config/BluetoothConfig.h"
#include "android/constants.h"
#include "android/cpu_accelerator.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/ParameterList.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/error-messages.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/feature_control.h"
#include "android/filesystems/ext4_resize.h"
#include "android/filesystems/ext4_utils.h"
#include "android/globals.h"
#include "android/help.h"
#include "android/kernel/kernel_utils.h"
#include "android/main-common-ui.h"
#include "android/main-common.h"
#include "android/main-kernel-parameters.h"
#include "android/multi-instance.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/opengles.h"
#include "android/process_setup.h"
#include "android/recording/screen-recorder.h"
#include "android/session_phase_reporter.h"
#include "android/snapshot/interface.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/filelock.h"
#include "android/utils/lineinput.h"
#include "android/utils/path.h"
#include "android/utils/property_file.h"
#include "android/utils/stralloc.h"
#include "android/utils/string.h"
#include "android/utils/tempfile.h"
#include "android/utils/win32_cmdline_quote.h"
#include "android/verified-boot/load_config.h"

#include "android/skin/qt/init-qt.h"
#include "android/skin/winsys.h"

#include "config-target.h"

extern "C" {
#include "android/skin/charmap.h"
#include "hw/misc/goldfish_pstore.h"
}

#include "android-qemu2-glue/dtb.h"
#include "android-qemu2-glue/emulation/serial_line.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/qemu-control-impl.h"
#include "android/ui-emu-agent.h"

#ifdef TARGET_AARCH64
#define TARGET_ARM64
#endif
#ifdef TARGET_I386
#define TARGET_X86
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <algorithm>

#include "android/version.h"
#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(init)) \
            dprint(__VA_ARGS__); \
    } while (0)

extern bool android_op_wipe_data;
extern bool android_op_writable_system;

// Check if we are running multiple emulators on the same AVD
static bool is_multi_instance = false;

using namespace android::base;
using android::base::System;
namespace fc = android::featurecontrol;

namespace {

enum ImageType {
    IMAGE_TYPE_SYSTEM = 0,
    IMAGE_TYPE_CACHE,
    IMAGE_TYPE_USER_DATA,
    IMAGE_TYPE_SD_CARD,
    IMAGE_TYPE_ENCRYPTION_KEY,
    IMAGE_TYPE_VENDOR,
    IMAGE_TYPE_MAX
};

const int kMaxPartitions = IMAGE_TYPE_MAX;
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
 * So far, we have 6(kMaxPartitions) types defined for system, cache, userdata
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
        {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_VENDOR, IMAGE_TYPE_ENCRYPTION_KEY,
         IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
        {NULL},
#elif defined(TARGET_ARM)
        "arm",
        "arm",
        "cortex-a15",
        "ttyAMA",
        "virtio-blk-device",
        "virtio-net-device",
        {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_VENDOR, IMAGE_TYPE_ENCRYPTION_KEY,
         IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
        {NULL},
#elif defined(TARGET_MIPS64)
        "mips64",
        "mips64el",
        "MIPS64R6-generic",
        "ttyGF",
        "virtio-blk-device",
        "virtio-net-device",
        {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_VENDOR, IMAGE_TYPE_ENCRYPTION_KEY,
         IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
        {NULL},
#elif defined(TARGET_MIPS)
        "mips",
        "mipsel",
        "74Kf",
        "ttyGF",
        "virtio-blk-device",
        "virtio-net-device",
        {IMAGE_TYPE_SD_CARD, IMAGE_TYPE_VENDOR, IMAGE_TYPE_ENCRYPTION_KEY,
         IMAGE_TYPE_USER_DATA, IMAGE_TYPE_CACHE, IMAGE_TYPE_SYSTEM},
        {NULL},
#elif defined(TARGET_X86_64)
        "x86_64",
        "x86_64",
        "android64",
        "ttyS",
        "virtio-blk-pci",
        "virtio-net-pci",
        {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA,
         IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_VENDOR, IMAGE_TYPE_SD_CARD},
        {"-vga", "none", NULL},
#elif defined(TARGET_I386)  // Both i386 and x86_64 targets define this macro
        "x86",
        "i386",
        "android32",
        "ttyS",
        "virtio-blk-pci",
        "virtio-net-pci",
        {IMAGE_TYPE_SYSTEM, IMAGE_TYPE_CACHE, IMAGE_TYPE_USER_DATA,
         IMAGE_TYPE_ENCRYPTION_KEY, IMAGE_TYPE_VENDOR, IMAGE_TYPE_SD_CARD},
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

/* Generate a hardware-qemu.ini for this AVD. The real hardware
 * configuration is ususally stored in several files, e.g. the AVD's
 * config.ini plus the skin-specific hardware.ini.
 *
 * The new file will group all definitions and will be used to
 * launch the core with the -android-hw <file> option.
 */
static int genHwIniFile(AndroidHwConfig* hw, const char* coreHwIniPath) {
    const auto hwIni = android::base::makeCustomScopedPtr(
            iniFile_newEmpty(NULL), iniFile_free);
    androidHwConfig_write(hw, hwIni.get());

    /* While saving HW config, ignore valueless entries. This will
     * not break anything, but will significantly simplify comparing
     * the current HW config with the one that has been associated
     * with a snapshot (in case VM starts from a snapshot for this
     * instance of emulator). */
    if (iniFile_saveToFileClean(hwIni.get(), coreHwIniPath) < 0) {
        derror("Could not write hardware.ini to %s: %s", coreHwIniPath,
               strerror(errno));
        return 2;
    }

    /* In verbose mode, dump the file's content */
    if (VERBOSE_CHECK(init)) {
        auto file = makeCustomScopedPtr(fopen(coreHwIniPath, "rt"), fclose);
        if (file.get() == NULL) {
            derror("Could not open hardware configuration file: "
                   "%s\n",
                   coreHwIniPath);
        } else {
            LineInput* input = lineInput_newFromStdFile(file.get());
            const char* line;
            printf("Content of hardware configuration file:\n");
            while ((line = lineInput_getLine(input)) != NULL) {
                printf("  %s\n", line);
            }
            printf(".\n");
            lineInput_free(input);
        }
    }

    return 0;
}

static void prepareDataFolder(const char* destDirectory,
                              const char* srcDirectory) {
    // The adb_keys file permission will also be set in guest system.
    // Referencing system/core/rootdir/init.usb.rc
    static const int kAdbKeyDirFilePerm = 02750;
    path_copy_dir(destDirectory, srcDirectory);
    std::string adbKeyPath = PathUtils::join(
            android::ConfigDirs::getUserDirectory(), "adbkey.pub");
    if (path_is_regular(adbKeyPath.c_str()) &&
        path_can_read(adbKeyPath.c_str())) {
        std::string guestAdbKeyDir =
                PathUtils::join(destDirectory, "misc", "adb");
        std::string guestAdbKeyPath =
                PathUtils::join(guestAdbKeyDir, "adb_keys");

        path_mkdir_if_needed(guestAdbKeyDir.c_str(), kAdbKeyDirFilePerm);
        path_copy_file(guestAdbKeyPath.c_str(), adbKeyPath.c_str());
        android_chmod(guestAdbKeyPath.c_str(), 0640);
    } else {
        dwarning("cannot read adb public key file: %s", adbKeyPath.c_str());
    }
}

static bool creatUserDataExt4Img(AndroidHwConfig* hw,
                                 const char* dataDirectory) {
    android_createExt4ImageFromDir(hw->disk_dataPartition_path, dataDirectory,
                                   android_hw->disk_dataPartition_size, "data");

    // Check if creating user data img succeed
    System::FileSize diskSize;
    if (System::get()->pathFileSize(hw->disk_dataPartition_path, &diskSize) &&
        diskSize > 0) {
        return true;
    } else {
        path_delete_file(hw->disk_dataPartition_path);
        return false;
    }
}

static int createUserData(AvdInfo* avd,
                          const char* dataPath,
                          AndroidHwConfig* hw) {
    ScopedCPtr<char> initDir(avdInfo_getDataInitDirPath(avd));
    bool needCopyDataPartition = true;
    if (path_exists(initDir.get())) {
        D("Creating ext4 userdata partition: %s", dataPath);
        prepareDataFolder(dataPath, initDir.get());
        needCopyDataPartition = !creatUserDataExt4Img(hw, dataPath);
        path_delete_dir(dataPath);
    }

    if (needCopyDataPartition) {
        if (path_exists(hw->disk_dataPartition_initPath)) {
            D("Creating: %s by copying from %s \n", hw->disk_dataPartition_path,
              hw->disk_dataPartition_initPath);

            if (path_copy_file(hw->disk_dataPartition_path,
                               hw->disk_dataPartition_initPath) < 0) {
                derror("Could not create %s: %s", hw->disk_dataPartition_path,
                       strerror(errno));
                return 1;
            }

            if (!hw->hw_arc) {
                resizeExt4Partition(android_hw->disk_dataPartition_path,
                                    android_hw->disk_dataPartition_size);
            }
        }
    }

    return 0;
}

static std::string get_qcow2_image_basename(const std::string& image) {
    char* basename = path_basename(image.c_str());
    std::string qcow2_basename(basename);
    free(basename);
    return qcow2_basename + ".qcow2";
}

/**
 * Class that's capable of creating that partition parameters
 */
class PartitionParameters {
public:
    static android::ParameterList create(AndroidHwConfig* hw, AvdInfo* avd) {
        return PartitionParameters(hw, avd).create();
    }

private:
    PartitionParameters(AndroidHwConfig* hw, AvdInfo* avd)
        : m_hw(hw), m_avd(avd), m_driveIndex(0) {}

    android::ParameterList create() {
        android::ParameterList args;
        for (auto type : kTarget.imagePartitionTypes) {
            bool writable =
                    (type == IMAGE_TYPE_SYSTEM || type == IMAGE_TYPE_VENDOR)
                            ? android_op_writable_system
                            : true;
            args.add(createPartionParameters(type, writable));
        }
        return args;
    }

    android::ParameterList createPartionParameters(ImageType type,
                                                   bool writable) {
        int apiLevel = avdInfo_getApiLevel(m_avd);

#if defined(TARGET_X86_64) || defined(TARGET_I386)
        /* for x86, 'if=none' is necessary for virtio blk*/
        std::string driveParam("if=none,");
#else
        std::string driveParam;
#endif

        std::string deviceParam;
        std::string bufferString;
        const char* qcow2Path;
        ScopedCPtr<const char> allocatedPath;
        StringView filePath;
        std::string sysImagePath, vendorImagePath;
        bool qCow2Format = true;
        bool needClone = false;
        switch (type) {
            case IMAGE_TYPE_SYSTEM:
                // API 15 and under images need a read+write system image.
                // API > 15 uses read-only system partition. You can override
                // this explicitly by passing -writable-system to emulator.
                if (apiLevel <= 15) {
                    writable = true;
                }
                sysImagePath = std::string(
                        avdInfo_getSystemImagePath(m_avd)
                                ?: avdInfo_getSystemInitImagePath(m_avd));
                if (writable) {
                    const char* systemDir = avdInfo_getContentPath(m_avd);

                    allocatedPath.reset(path_join(
                            systemDir,
                            get_qcow2_image_basename(sysImagePath).c_str()));
                    filePath = allocatedPath.get();
                    driveParam += StringFormat("index=%d,id=system,file=%s",
                                               m_driveIndex++, filePath);
                } else {
                    qCow2Format = false;
                    filePath = sysImagePath.c_str();
                    driveParam += StringFormat(
                            "index=%d,id=system,file=%s"
                            ",read-only",
                            m_driveIndex++, filePath);
                }
                deviceParam = StringFormat("%s,drive=system",
                                           kTarget.storageDeviceType);
                break;
            case IMAGE_TYPE_VENDOR:
                if (!m_hw->disk_vendorPartition_path &&
                    !m_hw->disk_vendorPartition_initPath) {
                    // we do not have a vendor image to mount
                    return {};
                }
                vendorImagePath = std::string(
                        avdInfo_getVendorImagePath(m_avd)
                                ?: avdInfo_getVendorInitImagePath(m_avd));
                if (writable) {
                    const char* systemDir = avdInfo_getContentPath(m_avd);
                    allocatedPath.reset(path_join(
                            systemDir,
                            get_qcow2_image_basename(vendorImagePath).c_str()));
                    filePath = allocatedPath.get();
                    driveParam += StringFormat("index=%d,id=vendor,file=%s",
                                               m_driveIndex++, filePath);
                } else {
                    qCow2Format = false;
                    filePath = vendorImagePath.c_str();
                    driveParam += StringFormat(
                            "index=%d,id=vendor,file=%s"
                            ",read-only",
                            m_driveIndex++, filePath);
                }
                deviceParam = StringFormat("%s,drive=vendor",
                                           kTarget.storageDeviceType);
                break;
            case IMAGE_TYPE_CACHE:
                filePath = m_hw->disk_cachePartition_path;
                bufferString = StringFormat("%s.qcow2", filePath);
                driveParam +=
                        StringFormat("index=%d,id=cache,file=%s",
                                     m_driveIndex++, bufferString.c_str());
                deviceParam = StringFormat("%s,drive=cache",
                                           kTarget.storageDeviceType);
                break;
            case IMAGE_TYPE_USER_DATA:
                filePath = m_hw->disk_dataPartition_path;
                bufferString = StringFormat("%s.qcow2", filePath);
                driveParam +=
                        StringFormat("index=%d,id=userdata,file=%s",
                                     m_driveIndex++, bufferString.c_str());
                deviceParam = StringFormat("%s,drive=userdata",
                                           kTarget.storageDeviceType);
                break;
            case IMAGE_TYPE_SD_CARD:
                if (m_hw->hw_sdCard_path != NULL &&
                    strcmp(m_hw->hw_sdCard_path, "")) {
                    filePath = m_hw->hw_sdCard_path;
                    bufferString = StringFormat("%s.qcow2", filePath);
                    driveParam +=
                            StringFormat("index=%d,id=sdcard,file=%s",
                                         m_driveIndex++, bufferString.c_str());
                    deviceParam = StringFormat("%s,drive=sdcard",
                                               kTarget.storageDeviceType);
                } else {
                    /* no sdcard is defined */
                    return {};
                }
                break;
            case IMAGE_TYPE_ENCRYPTION_KEY:
                if (fc::isEnabled(fc::EncryptUserData) &&
                    m_hw->disk_encryptionKeyPartition_path != NULL &&
                    strcmp(m_hw->disk_encryptionKeyPartition_path, "")) {
                    filePath = m_hw->disk_encryptionKeyPartition_path;
                    bufferString = StringFormat("%s.qcow2", filePath);
                    driveParam +=
                            StringFormat("index=%d,id=encrypt,file=%s",
                                         m_driveIndex++, bufferString.c_str());
                    deviceParam = StringFormat("%s,drive=encrypt",
                                               kTarget.storageDeviceType);
                } else {
                    /* no encryption partition is defined */
                    return {};
                }
                break;
            default:
                dwarning("Unknown Image type %d\n", type);
                return {};
        }

        if (qCow2Format) {
            // Disable extra qcow2 checks as we're on its stable version.
            // Disable cache flushes as well, as Android issues way too many
            // flush commands for nothing.
            driveParam += ",overlap-check=none,cache=unsafe";

            // Default qcow2's L2 cache size is up to 8GB. Let's increase it for
            // larger images.
            System::FileSize diskSize;
            if (System::get()->pathFileSize(filePath, &diskSize)) {
                // L2 cache size should be "disk_size_GB / 131072" as per QEMU
                // docs with a default of 1MB. Round it up just in case.
                const int l2CacheSize =
                        std::max<int>((diskSize + (1024 * 1024 * 1024 - 1)) /
                                              (1024 * 1024 * 1024) * 131072,
                                      1024 * 1024);
                driveParam += StringFormat(",l2-cache-size=%d", l2CacheSize);
            }
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

        return {"-drive", driveParam, "-device", deviceParam};
    }

private:
    AndroidHwConfig* m_hw;
    AvdInfo* m_avd;
    int m_driveIndex;
};

}  // namespace

extern "C" int run_qemu_main(int argc,
                             const char** argv,
                             void (*on_main_loop_done)(void));

static void enter_qemu_main_loop(int argc, char** argv) {
#ifndef _WIN32
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif
// stick a version here for qemu-system binary
#if defined ANDROID_SDK_TOOLS_BUILD_NUMBER
    D("Android qemu version %s (CL:%s)\n",
      EMULATOR_VERSION_STRING
      " (build_id " STRINGIFY(ANDROID_SDK_TOOLS_BUILD_NUMBER) ")",
      EMULATOR_CL_SHA1);
#endif

    D("Starting QEMU main loop");
    run_qemu_main(argc, (const char**)argv, [] {
        skin_winsys_run_ui_update(
                [](void*) {
                    // It is only safe to stop the OpenGL ES renderer after the
                    // main loop has exited. This is not necessarily before
                    // |skin_window_free| is called, especially on Windows!
                    android_stopOpenglesRenderer(false);
                },
                nullptr, false);
    });
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
    ScopedCPtr<char> userdata_dir(path_dirname(hw->disk_dataPartition_path));
    if (!userdata_dir) {
        derror("no userdata_dir");
        return false;
    }
    hw->disk_encryptionKeyPartition_path =
            path_join(userdata_dir.get(), "encryptionkey.img");
    if (path_exists(hw->disk_systemPartition_initPath)) {
        ScopedCPtr<char> sysimg_dir(
                path_dirname(hw->disk_systemPartition_initPath));
        if (!sysimg_dir.get()) {
            derror("no sysimg_dir %s", hw->disk_systemPartition_initPath);
            return false;
        }
        ScopedCPtr<char> init_encryptionkey_img_path(
                path_join(sysimg_dir.get(), "encryptionkey.img"));
        if (path_exists(init_encryptionkey_img_path.get())) {
            if (path_copy_file(hw->disk_encryptionKeyPartition_path,
                               init_encryptionkey_img_path.get()) >= 0) {
                return true;
            }
        } else {
            derror("no init encryptionkey.img");
        }
    } else {
        derror("no system partition %s", hw->disk_systemPartition_initPath);
    }
    return false;
}

extern AndroidProxyCB* gAndroidProxyCB;
extern "C" int main(int argc, char** argv) {
    if (argc < 1) {
        fprintf(stderr, "Invalid invocation (no program path)\n");
        return 1;
    }

    process_early_setup(argc, argv);
    android_report_session_phase(ANDROID_SESSION_PHASE_PARSEOPTIONS);

    // Start GPU information query to use it later for the renderer seleciton.
    async_query_host_gpu_start();

    const char* executable = argv[0];
    android::ParameterList args = {executable};
    AndroidHwConfig* hw = android_hw;
    AvdInfo* avd;
    AndroidOptions opts[1];
    int exitStatus = 0;

    gAndroidProxyCB->ProxySet = qemu_android_setup_http_proxy;
    gAndroidProxyCB->ProxyUnset = qemu_android_remove_http_proxy;
    qemu_android_init_http_proxy_ops();

    if (!emulator_parseCommonCommandLineOptions(
                &argc, &argv, kTarget.androidArch,
                true,  // is_qemu2
                opts, hw, &android_avdInfo, &exitStatus)) {
        // Special case for QEMU positional parameters.
        if (exitStatus == EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER) {
            // Copy all QEMU options to |args|, and set |n| to the number
            // of options in |args| (|argc| must be positive here).
            // NOTE: emulator_parseCommonCommandLineOptions has side effects
            // and modififes argc, as well as argv. Because of these magical
            // side effects we are *NOT* just copying over argc, argv.
            for (int n = 1; n <= argc; ++n) {
                args.add(argv[n - 1]);
            }

            // Skip the translation of command-line options and jump
            // straight to qemu_main().
            enter_qemu_main_loop(args.size(), args.array());
            return 0;
        }

        // Normal exit.
        return exitStatus;
    }

    // just because we know that we're in the new emulator as we got here
    opts->ranchu = 1;

    avd = android_avdInfo;

    bool lowDisk = System::isUnderDiskPressure(avdInfo_getContentPath(avd));
    if (lowDisk) {
        derror("Not enough disk space to run AVD '%s'. Exiting...\n",
               avdInfo_getName(avd));
        return 1;
    }

    if (avdInfo_inAndroidBuild(avd) || opts->read_only) {
        android::base::disableRestart();
    } else {
        android::base::finalizeEmulatorRestartParameters(avdInfo_getContentPath(avd));
    }

    // Lock the AVD as soon as we can to make sure other copy won't do anything
    // stupid before detecting that the AVD is already in use.
    const char* coreHwIniPath = avdInfo_getCoreHwIniPath(avd);

    // Before that, check for a snapshot lock to see if there is any pending
    // snapshot operation, in which case we just wait it out.
    const char* snapshotLockFilePath = avdInfo_getSnapshotLockFilePath(avd);
    // 10 seconds
    FileLock* snapshotLock =
            filelock_create_timeout(snapshotLockFilePath, 10000 /* ms */);
    if (!snapshotLock) {
        // Some snapshot operation took too long.
        derror("A snapshot operation for '%s' is pending "
               "and timeout has expired. Exiting...\n",
               avdInfo_getName(avd));
        return 1;
    }
    if (opts->read_only) {
        TempFile*  tempIni = tempfile_create();
        coreHwIniPath = tempfile_path(tempIni);
        is_multi_instance = true;
        opts->no_snapshot_save = true;
        args.add("-read-only");
    } else if (filelock_create_timeout(coreHwIniPath, 2000) == NULL) {
        /* The AVD is already in use, we still support this as an
         * experimental feature. Use a temporary hardware-qemu.ini
         * file though to avoid overwriting the existing one. */
        derror("Running multiple emulators with the same AVD "
               "is an experimental feature.\n"
               "Please use -read-only flag to enable this feature.\n");
        return 1;
    }

    android::base::FileShare shareMode = opts->read_only ? FileShare::Read
                                                         : FileShare::Write;
    if (!android::multiinstance::initInstanceShareMode(shareMode)) {
        return 1;
    }

    if (snapshotLock) {
        filelock_release(snapshotLock);
    }

    // Update server-based hw config / feature flags.
    // Must be done after emulator_parseCommonCommandLineOptions,
    // since that calls createAVD which sets up critical info needed
    // by featurecontrol component itself.
#if (SNAPSHOT_PROFILE > 1)
    printf("Starting feature flag application and host hw query with uptime "
           "%" PRIu64 " ms\n",
           get_uptime_ms());
#endif
    feature_initialize();
    feature_update_from_server();
#if (SNAPSHOT_PROFILE > 1)
    printf("Finished feature flag application and host hw query with uptime "
           "%" PRIu64 " ms\n",
           get_uptime_ms());
#endif

    if (!emulator_parseFeatureCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    if (!emulator_parseUiCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    // If the virtual scene camera is selected in the avd, but not supported,
    // use the emulated camera instead.
    const bool isVirtualScene = !strcmp(hw->hw_camera_back, "virtualscene");
    if (isVirtualScene && !feature_is_enabled(kFeature_VirtualScene)) {
        str_reset(&hw->hw_camera_back, "emulated");
    }

    // Parse virtual scene command line options, if enabled.
    camera_virtualscene_parse_cmdline();

    if (opts->shared_net_id) {
        char* end;
        long shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            fprintf(stderr,
                    "option -shared-net-id must be an integer between 1 and "
                    "255\n");
            return 1;
        }
        args.add("-boot-property");
        args.addFormat("net.shared_net_ip=10.1.2.%ld", shared_net_id);
    }

    // Add bluetooth parameters if applicable.
    char* bluetooth_opts = NULL;
#ifdef __linux__
    bluetooth_opts = opts->bluetooth;

#endif
    android::BluetoothConfig bluetooth(bluetooth_opts);
    args.add(bluetooth.getParameters());

#ifdef CONFIG_NAND_LIMITS
    args.add2If("-nand-limits", opts->nand_limits);
#endif

    args.add2If("-timezone", opts->timezone);
    args.add2If("-cpu-delay", opts->cpu_delay);
    args.add2If("-dns-server", opts->dns_server);
    args.addIf("-skip-adb-auth", opts->skip_adb_auth);

    if (opts->audio && !strcmp(opts->audio, "none"))
        args.add("-no-audio");

    bool badSnapshots = false;

    // Check situations where snapshots should be turned off
    {
        // Just dont' use snapshots on 32 bit - crashes galore
        badSnapshots = badSnapshots || (System::get()->getProgramBitness() == 32);

        // Bad generic snapshots command line option
        if (opts->snapshot && opts->snapshot[0] == '\0') {
            opts->snapshot = nullptr;
            opts->no_snapshot_load = true;
            opts->no_snapshot_save = true;
            badSnapshots = true;
        } else if (opts->snapshot) {
            // Never save snapshot on exit if we are booting with a snapshot;
            // it will overwrite quickboot state
            opts->no_snapshot_save = true;
        }

        if (badSnapshots) {
            feature_set_enabled_override(kFeature_FastSnapshotV1, false);
            feature_set_enabled_override(kFeature_QuickbootFileBacked, false);
        }
    }

    if (opts->snapshot && feature_is_enabled(kFeature_FastSnapshotV1)) {
        if (!opts->no_snapshot_load) {
            args.add2("-loadvm", opts->snapshot);
        }
    }

    bool useQuickbootRamFile =
        feature_is_enabled(kFeature_QuickbootFileBacked) &&
        !opts->snapshot;

    if (useQuickbootRamFile) {
        ScopedCPtr<const char> memPath(
            androidSnapshot_prepareAutosave(hw->hw_ramSize, nullptr));

        if (memPath) {
            args.add2("-mem-path", memPath.get());

            bool mapAsShared =
                !opts->read_only &&
                !opts->snapshot &&
                !opts->no_snapshot_save &&
                androidSnapshot_getQuickbootChoice();

            if (mapAsShared) {
                args.add("-mem-file-shared");
                androidSnapshot_setRamFileDirty(nullptr, true);
            }
        } else {
            fprintf(stderr, "Warning: could not initialize Quickboot RAM file. "
                            "Please ensure enough disk space for the guest RAM size "
                            "(%d MB) along with a safety factor.\n", hw->hw_ramSize);
            feature_set_enabled_override(kFeature_QuickbootFileBacked, false);
        }
    }

    /** SNAPSHOT STORAGE HANDLING */

    if (opts->snapshot_list) {
        args.add("-snapshot-list");
    }

    /* If we have a valid snapshot storage path */

    if (opts->snapstorage) {
        // NOTE: If QEMU2_SNAPSHOT_SUPPORT is not defined, a warning has been
        //       already printed by emulator_parseCommonCommandLineOptions().
#ifdef QEMU2_SNAPSHOT_SUPPORT
        /* We still use QEMU command-line options for the following since
         * they can change from one invokation to the next and don't really
         * correspond to the hardware configuration itself.
         */
        if (!opts->no_snapshot_save)
            args.add2("-savevm-on-exit", opts->snapshot);
        if (opts->no_snapshot_update_time)
            args.add("-snapshot-no-time-update");
#endif  // QEMU2_SNAPSHOT_SUPPORT
    }

    if (fc::isEnabled(fc::LogcatPipe) && opts->logcat) {
        boot_property_add_logcat_pipe(opts->logcat);
        // we have done with -logcat option.
        opts->logcat = NULL;
    }

    // Always setup a single serial port, that can be connected
    // either to the 'null' chardev, or the -shell-serial one,
    // which by default will be either 'stdout' (Posix) or 'con:'
    // (Windows).
    const char* serial = (opts->shell || opts->logcat || opts->show_kernel)
                                 ? opts->shell_serial
                                 : "null";
    args.add2("-serial", serial);

    args.add2If("-radio", opts->radio);
    args.add2If("-gps", opts->gps);
    args.add2If("-code-profile", opts->code_profile);


    /* Pass boot properties to the core. First, those from boot.prop,
     * then those from the command-line */
    const FileData* bootProperties = avdInfo_getBootProperties(avd);
    if (!fileData_isEmpty(bootProperties)) {
        PropertyFileIterator iter[1];
        propertyFileIterator_init(iter, bootProperties->data,
                                  bootProperties->size);
        while (propertyFileIterator_next(iter)) {
            args.add("-boot-property");
            args.addFormat("%s=%s", iter->name, iter->value);
        }
    }

    for (ParamList* pl = opts->prop; pl != NULL; pl = pl->next) {
        args.add2("-boot-property", pl->param);
    }

    args.add2If("-android-ports", opts->ports);
    if (opts->port) {
        int console_port = -1;
        int adb_port = -1;
        if (!android_parse_port_option(opts->port, &console_port, &adb_port)) {
            return 1;
        }
        args.add("-android-ports");
        args.addFormat("%d,%d", console_port, adb_port);
    }

    args.add2If("-android-report-console", opts->report_console);

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
        /* We need to store the charmap name in the hardware
         * configuration. However, the charmap file itself is only used
         * by the UI component and doesn't need to be set to the
         * emulation engine.
         */
        kcm_extract_charmap_name(opts->charmap, charmap_name,
                                 sizeof(charmap_name));
        str_reset(&hw->hw_keyboard_charmap, charmap_name);
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

    android_report_session_phase(ANDROID_SESSION_PHASE_INITGENERAL);
    // Initialize a persistent ram block.
    // dont touch original data folder in build environment
    std::string dataPath = PathUtils::join(avdInfo_getContentPath(avd),
            avdInfo_inAndroidBuild(avd) ? "build.avd/data" : "data");
    std::string pstorePath = PathUtils::join(dataPath, "misc", "pstore");
    std::string pstoreFile = PathUtils::join(pstorePath, "pstore.bin");
    if (android_op_wipe_data) {
        path_delete_file(pstoreFile.c_str());
    }
    path_mkdir_if_needed(pstorePath.c_str(), 0777);
    android_chmod(pstorePath.c_str(), 0777);

    mem_map pstore = {.start = GOLDFISH_PSTORE_MEM_BASE,
                      .size = GOLDFISH_PSTORE_MEM_SIZE};

    args.add("-device");
    args.addFormat("goldfish_pstore,addr=0x%" PRIx64 ",size=0x%" PRIx64
                   ",file=%s",
                   pstore.start, pstore.size, pstoreFile.c_str());

    // studio avd manager does not allow user to change
    // partition size, set a lower limit to 2GB
    constexpr auto kMinPlaystoreImageSize = 2LL * 1024 * 1024 * 1024;
    if (fc::isEnabled(fc::PlayStoreImage)) {
        if (android_hw->disk_dataPartition_size < kMinPlaystoreImageSize) {
            android_hw->disk_dataPartition_size = kMinPlaystoreImageSize;
        }
    }

    // Create userdata file from init version if needed.
    if ((android_op_wipe_data || !path_exists(hw->disk_dataPartition_path))) {
        int ret = createUserData(avd, dataPath.c_str(), hw);
        if (ret != 0) {
            crashhandler_die("Failed to initialize userdata.img.");
            return ret;
        }
    } else if (!hw->hw_arc) {
        // Resize userdata-qemu.img if the size is smaller than what
        // config.ini says and also delete userdata-qemu.img.qcow2.
        // This can happen as user wants a larger data
        // partition without wiping it. b.android.com/196926
        System::FileSize current_data_size;
        if (System::get()->pathFileSize(hw->disk_dataPartition_path,
                                        &current_data_size)) {
            System::FileSize partition_size = static_cast<System::FileSize>(
                    android_hw->disk_dataPartition_size);
            if (android_hw->disk_dataPartition_size > 0 &&
                current_data_size < partition_size) {
                dwarning(
                        "userdata partition is resized from %d M to %d "
                        "M\n",
                        (int)(current_data_size / (1024 * 1024)),
                        (int)(partition_size / (1024 * 1024)));
                if (!resizeExt4Partition(android_hw->disk_dataPartition_path,
                                    android_hw->disk_dataPartition_size)) {
                    path_delete_file(StringFormat("%s.qcow2", android_hw->disk_dataPartition_path).c_str());
                }
            }
        }
    }

    // create encryptionkey.img file if needed
    if (fc::isEnabled(fc::EncryptUserData)) {
        if (hw->disk_encryptionKeyPartition_path == NULL) {
            if (!createInitalEncryptionKeyPartition(hw)) {
                derror("Encryption is requested but failed to create "
                       "encrypt "
                       "partition.");
                return 1;
            }
        }
    } else {
        dwarning("encryption is off");
    }

    bool createEmptyCacheFile = false;

    // Make sure there's a temp cache partition if there wasn't a permanent one
    if ((!hw->disk_cachePartition_path ||
         strcmp(hw->disk_cachePartition_path, "") == 0) &&
        !hw->hw_arc) {
        str_reset(&hw->disk_cachePartition_path,
                  tempfile_path(tempfile_create()));
        createEmptyCacheFile = true;
    }

    if (!path_exists(hw->disk_cachePartition_path) && !hw->hw_arc) {
        createEmptyCacheFile = true;
    }

    if (createEmptyCacheFile) {
        D("Creating empty ext4 cache partition: %s",
          hw->disk_cachePartition_path);
        int ret = android_createEmptyExt4Image(hw->disk_cachePartition_path,
                                               hw->disk_cachePartition_size,
                                               "cache");
        if (ret < 0) {
            derror("Could not create %s: %s", hw->disk_cachePartition_path,
                   strerror(-ret));
            return 1;
        }
    }

    android_report_session_phase(ANDROID_SESSION_PHASE_INITACCEL);

    // Make sure we always use the custom Android CPU definition.
    args.add("-cpu");
#if defined(TARGET_MIPS)
    args.add((hw->hw_cpu_model && hw->hw_cpu_model[0]) ? hw->hw_cpu_model
                                                       : kTarget.qemuCpu);
#else
    args.add(kTarget.qemuCpu);
#endif

    // Set env var to "on" for Intel PMU if the feature is enabled.
    // cpu.c will then read that.
    if (fc::isEnabled(fc::IntelPerformanceMonitoringUnit)) {
        System::get()->envSet(
                "ANDROID_EMU_FEATURE_IntelPerformanceMonitoringUnit", "on");
    }

#if defined(TARGET_X86_64) || defined(TARGET_I386)
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    const bool accel_ok =
            handleCpuAcceleration(opts, avd, &accel_mode, &accel_status);
    AndroidCpuAccelerator accelerator = androidCpuAcceleration_getAccelerator();
    const char* enableAcceleratorParam = getAcceleratorEnableParam(accelerator);

    if (accel_mode == ACCEL_ON) {  // 'accel on' is specified'
        if (!accel_ok) {
            derror("CPU acceleration is not supported on this "
                   "machine!");
            derror("Reason: %s", accel_status);
            AFREE(accel_status);
            return 1;
        }
        args.add(enableAcceleratorParam);
    } else if (accel_mode == ACCEL_AUTO) {
        if (accel_ok) {
            args.add(enableAcceleratorParam);
        }
    } else if (accel_mode == ACCEL_HVF) {
#if CONFIG_HVF
        args.add(enableAcceleratorParam);
#endif
    }  // else, add other special situations to enable particular
       // acceleration backends (e.g., HyperV/KVM on Windows,
       // KVM on Mac, etc.)

    AFREE(accel_status);
#else   // !TARGET_X86_64 && !TARGET_I386
    args.add2("-machine", "type=ranchu");
#endif  // !TARGET_X86_64 && !TARGET_I386

#if defined(TARGET_X86_64) || defined(TARGET_I386)
    // SMP Support.
    if (hw->hw_cpu_ncore > 1 &&
        !androidCpuAcceleration_hasModernX86VirtualizationFeatures()) {
        dwarning(
                "Not all modern X86 virtualization features supported, which "
                "introduces problems with slowdown when running Android on "
                "multicore vCPUs. Setting AVD to run with 1 vCPU core only.");
        hw->hw_cpu_ncore = 1;
    }

    if (hw->hw_cpu_ncore > 1) {
        args.add("-smp");

#ifdef _WIN32
        if (hw->hw_cpu_ncore > 16) {
            dwarning(
                    "HAXM does not support more than 16 cores. Number of cores "
                    "set to 16");
            hw->hw_cpu_ncore = 16;
        }
#endif
        args.addFormat("cores=%d", hw->hw_cpu_ncore);
    }
#endif  // !TARGET_X86_64 && !TARGET_I386

    // Memory size
    args.add("-m");
    args.addFormat("%d", hw->hw_ramSize);

    int apiLevel = avd ? avdInfo_getApiLevel(avd) : 1000;

    // Support for changing default lcd-density
    if (hw->hw_lcd_density) {
        args.add("-lcd-density");
        args.addFormat("%d", hw->hw_lcd_density);
    }

    // Kernel image, ramdisk

// Dedicated IOThread for all disk IO
#if defined(CONFIG_LINUX) && (defined(TARGET_X86_64) || defined(TARGET_I386))
    args.add2("-object", "iothread,id=disk-iothread");
#endif

    // Don't create the default CD drive and floppy disk devices - Android
    // won't appreciate it.
    args.add("-nodefaults");

    if (!hw->hw_arc) {
        args.add(
                {"-kernel", hw->kernel_path, "-initrd", hw->disk_ramdisk_path});
        // add partition parameters with the sequence pre-defined in
        // targetInfo.imagePartitionTypes
        args.add(PartitionParameters::create(hw, avd));
    } else {
        args.add2("-kernel", hw->kernel_path);
        // hw->hw_arc: ChromeOS single disk image, use regular block device
        // instead of virtio block device
        args.add("-drive");
        const char* avd_dir = avdInfo_getContentPath(avd);
        args.addFormat("format=raw,file=cat:%s" PATH_SEP
                       "system.img.qcow2|"
                       "%s" PATH_SEP
                       "userdata-qemu.img.qcow2|"
                       "%s" PATH_SEP "vendor.img.qcow2",
                       avd_dir, avd_dir, avd_dir);
    }

    if (fc::isEnabled(fc::KernelDeviceTreeBlobSupport)) {
        ScopedCPtr<char> userdata_dir(path_dirname(hw->disk_dataPartition_path));
        assert(userdata_dir);

        const std::string dtbFileName =
            PathUtils::join(userdata_dir.get(), "default.dtb");

        if (android_op_wipe_data || !path_exists(dtbFileName.c_str())) {
            ::dtb::Params params;

            char *vendor_path = avdInfo_getVendorImageDevicePathInGuest(avd);
            if (vendor_path) {
                params.vendor_device_location = vendor_path;
                free(vendor_path);

                exitStatus = createDtbFile(params, dtbFileName);
                if (exitStatus) {
                    derror("Could not create a DTB file (%s)", dtbFileName.c_str());
                    return exitStatus;
                }
            } else {
                derror("No vendor path found");
                return 1;
            }
        }
        args.add({"-dtb", dtbFileName});
    }

    // Network
    args.add("-netdev");
    if (opts->net_tap) {
        const char* upScript =
                opts->net_tap_script_up ? opts->net_tap_script_up : "no";
        const char* downScript =
                opts->net_tap_script_down ? opts->net_tap_script_down : "no";
        args.addFormat("tap,id=mynet,script=%s,downscript=%s,ifname=%s",
                       upScript, downScript, opts->net_tap);
    } else {
        if (opts->net_tap_script_up) {
            dwarning("-net-tap-script-up ignored without -net-tap option");
        }
        if (opts->net_tap_script_down) {
            dwarning("-net-tap-script-down ignored without -net-tap option");
        }
        args.add("user,id=mynet");
    }
    args.add("-device");
    args.addFormat("%s,netdev=mynet", kTarget.networkDeviceType);

    // rng
#if defined(TARGET_X86_64) || defined(TARGET_I386)
    args.add("-device");
    args.add("virtio-rng-pci");
#else
    args.add("-device");
    args.add("virtio-rng-device");
#endif
    args.add("-show-cursor");

    if (opts->tcpdump) {
        args.add("-object");
        args.addFormat("filter-dump,id=mytcpdump,netdev=mynet,file=%s",
                       opts->tcpdump);
    }

    // Graphics
    if (opts->no_window) {
        args.add("-nographic");
        // also disable the qemu monitor which will otherwise grab stdio
        args.add2("-monitor", "none");
    }

    // Data directory (for keymaps and PC Bios).
    args.add("-L");
    std::string dataDir = getNthParentDir(executable, 3U);
    if (dataDir.empty()) {
        dataDir = "lib/pc-bios";
    } else {
        dataDir += "/lib/pc-bios";
    }
    args.add(dataDir);

// Audio enable hda by default for x86 and x64 platforms
#if defined(TARGET_X86_64) || defined(TARGET_I386)
    args.add2("-soundhw", "hda");
#endif

    /* append extra qemu parameters if any */
    for (int idx = 0; kTarget.qemuExtraArgs[idx] != NULL; idx++) {
        args.add(kTarget.qemuExtraArgs[idx]);
    }

    android_report_session_phase(ANDROID_SESSION_PHASE_INITGPU);

    if (gQAndroidBatteryAgent && gQAndroidBatteryAgent->setHasBattery) {
        gQAndroidBatteryAgent->setHasBattery(android_hw->hw_battery);
    }

    gQAndroidLocationAgent->gpsSetPassiveUpdate(!opts->no_passive_gps);

    // Setup GPU acceleration. This needs to go along with user interface
    // initialization, because we need the selected backend from Qt settings.
    const UiEmuAgent uiEmuAgent = {
            gQAndroidBatteryAgent,
            gQAndroidCellularAgent,
            gQAndroidClipboardAgent,
            gQAndroidDisplayAgent,
            gQAndroidEmulatorWindowAgent,
            gQAndroidFingerAgent,
            gQAndroidLocationAgent,
            gQAndroidHttpProxyAgent,
            gQAndroidRecordScreenAgent,
            gQAndroidSensorsAgent,
            gQAndroidTelephonyAgent,
            gQAndroidUserEventAgent,
            gQAndroidVirtualSceneAgent,
            gQCarDataAgent,
            nullptr  // For now there's no uses of SettingsAgent, so we
                     // don't set it.
    };

    {
        qemu2_android_serialline_init();

        /* Setup SDL UI just before calling the code */
        android::base::Thread::maskAllSignals();

#if (SNAPSHOT_PROFILE > 1)
        printf("skin_winsys_init and UI starting at uptime %" PRIu64 " ms\n",
               get_uptime_ms());
#endif
        skin_winsys_init_args(argc, argv);
        if (!emulator_initUserInterface(opts, &uiEmuAgent)) {
            return 1;
        }
        if (opts->ui_only) {
            // UI only. emulator_initUserInterface() is done, so we're done.
            return 0;
        }

        // Register the quit callback
        android::base::registerEmulatorQuitCallback([] {
            android::base::ThreadLooper::runOnMainLooper([] {
                skin_winsys_quit_request();
            });
        });

#if (SNAPSHOT_PROFILE > 1)
        printf("skin_winsys_init and UI finishing at uptime %" PRIu64 " ms\n",
               get_uptime_ms());
#endif

        // Use advancedFeatures to override renderer if the user has
        // selected in UI that the preferred renderer is "autoselected".
        WinsysPreferredGlesBackend uiPreferredGlesBackend =
                skin_winsys_get_preferred_gles_backend();
#ifndef _WIN32
        if (uiPreferredGlesBackend == WINSYS_GLESBACKEND_PREFERENCE_ANGLE ||
            uiPreferredGlesBackend == WINSYS_GLESBACKEND_PREFERENCE_ANGLE9) {
            uiPreferredGlesBackend = WINSYS_GLESBACKEND_PREFERENCE_AUTO;
            skin_winsys_set_preferred_gles_backend(uiPreferredGlesBackend);
        }
#endif

        // Feature flags-related last-microsecond renderer changes
        {
            // Should enable OpenGL ES 3.x?
            if (skin_winsys_get_preferred_gles_apilevel() ==
                WINSYS_GLESAPILEVEL_PREFERENCE_MAX) {
                fc::setIfNotOverridenOrGuestDisabled(fc::GLESDynamicVersion,
                                                     true);
            }
            if (skin_winsys_get_preferred_gles_apilevel() ==
                        WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT ||
                System::get()->getProgramBitness() == 32) {
                fc::setEnabledOverride(fc::GLESDynamicVersion, false);
            }

            if (fc::isEnabled(fc::ForceANGLE)) {
                uiPreferredGlesBackend =
                        skin_winsys_override_glesbackend_if_auto(
                                WINSYS_GLESBACKEND_PREFERENCE_ANGLE);
            }

            if (fc::isEnabled(fc::ForceSwiftshader)) {
                uiPreferredGlesBackend =
                        skin_winsys_override_glesbackend_if_auto(
                                WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER);
            }
        }

        RendererConfig rendererConfig;
        configAndStartRenderer(avd, opts, hw, uiPreferredGlesBackend,
                               &rendererConfig);

        // Gpu configuration is set, now initialize the screen recorder
        // and screenshot callback
        bool isGuestMode =
                (!hw->hw_gpu_enabled || !strcmp(hw->hw_gpu_mode, "guest"));
        screen_recorder_init(hw->hw_lcd_width, hw->hw_lcd_height,
                             isGuestMode ? uiEmuAgent.display : nullptr);
        android_registerScreenshotFunc([](const char* dirname) {
            android::emulation::captureScreenshot(dirname, nullptr);
        });

        /* Disable the GLAsyncSwap for ANGLE so far */
        bool shouldDisableAsyncSwap =
                rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE ||
                rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE9;
        // Features to disable or enable depending on rendering backend
        // and gpu make/model/version
        shouldDisableAsyncSwap |= !strncmp("arm", kTarget.androidArch, 3) ||
                                  System::get()->getProgramBitness() == 32;
        shouldDisableAsyncSwap = shouldDisableAsyncSwap ||
                                 async_query_host_gpu_SyncBlacklisted();

        if (shouldDisableAsyncSwap) {
            fc::setEnabledOverride(fc::GLAsyncSwap, false);
        }

        // Get verified boot kernel parameters, if they exist.
        // If this is not a playstore image, then -writable_system will
        // disable verified boot.
        std::vector<std::string> verified_boot_params;
        if (feature_is_enabled(kFeature_PlayStoreImage) || !android_op_writable_system) {
          android::verifiedboot::getParametersFromFile(
                  avdInfo_getVerifiedBootParamsPath(avd),  // NULL here is OK
                  &verified_boot_params);
        }

        ScopedCPtr<char> kernel_parameters(emulator_getKernelParameters(
                opts, kTarget.androidArch, apiLevel, kTarget.ttyPrefix,
                hw->kernel_parameters, &verified_boot_params,
                rendererConfig.glesMode, rendererConfig.bootPropOpenglesVersion,
                rendererConfig.glFramebufferSizeBytes, pstore, hw->vm_heapSize,
                true /* isQemu2 */, hw->hw_arc));

        if (!kernel_parameters.get()) {
            return 1;
        }

        /* append the kernel parameters after -qemu */
        std::string append_arg(kernel_parameters.get());
        for (int i = 0; i < argc; ++i) {
            if (!strcmp(argv[i], "-append")) {
                if (++i < argc) {
                    android::base::StringAppendFormat(&append_arg, " %s",
                                                      argv[i]);
                }
            } else {
                args.add(argv[i]);
            }
        }

        if (hw->hw_arc) {
            /* We don't use goldfish_fb in cros. Just use virtio vga now */
            args.add2("-vga", "virtio");

            /* We don't use goldfish_events for touch events in cros.
             * Just use usb device now.
             */
            args.add2("-usbdevice", "tablet");
            if (!isGuestMode) args.add2("-display", "sdl,gl=on");
        }

        args.add(bluetooth.getQemuParameters());

        args.add("-append");
        args.add(append_arg);
    }

    android_report_session_phase(ANDROID_SESSION_PHASE_INITGENERAL);

    // Generate a hardware-qemu.ini for this AVD.
    int ret = genHwIniFile(hw, coreHwIniPath);
    if (ret != 0)
        return ret;

    args.add2("-android-hw", coreHwIniPath);
    crashhandler_copy_attachment(CRASH_AVD_HARDWARE_INFO, coreHwIniPath);

    if (VERBOSE_CHECK(init)) {
        printf("QEMU options list:\n");
        for (int i = 0; i < args.size(); i++) {
            printf("emulator: argv[%02d] = \"%s\"\n", i, args[i].c_str());
        }
        // Dump final command-line option to make debugging the core easier
        printf("Concatenated QEMU options:\n %s\n", args.toString().c_str());
    }

    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop, args.size(),
                             args.array());
    android::crashreport::CrashReporter::get()->hangDetector().pause(false);
    skin_winsys_enter_main_loop(opts->no_window);
    android::crashreport::CrashReporter::get()->hangDetector().pause(true);

    stopRenderer();
    emulator_finiUserInterface();

    process_late_teardown();
    return 0;
}
