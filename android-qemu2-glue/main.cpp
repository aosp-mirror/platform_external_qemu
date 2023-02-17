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

#include "android-qemu2-glue/qemu-console-factory.h"
#include "android/android.h"
#include "android/avd/hw-config.h"
#include "android/base/ProcessControl.h"
#include "android/base/StringFormat.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32Utils.h"
#include "android/base/threads/Async.h"
#include "android/base/threads/Thread.h"
#include "android/boot-properties.h"
#include "android/bootconfig.h"
#include "android/camera/camera-virtualscene.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/constants.h"
#include "android/cpu_accelerator.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/HostapdController.h"
#include "android/emulation/LogcatPipe.h"
#include "android/emulation/MultiDisplay.h"
#include "android/emulation/ParameterList.h"
#include "android/emulation/USBAssist.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/adb/adbkey.h"
#include "android/emulation/control/automation_agent.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/error-messages.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/feature_control.h"
#include "android/filesystems/ext4_resize.h"
#include "android/filesystems/ext4_utils.h"
#include "android/help.h"
#include "android/hw-sensors.h"
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
#include "android/snapshot/check_snapshot_loadable.h"
#include "android/snapshot/interface.h"
#include "android/userspace-boot-properties.h"
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

// modem simulator
extern "C" {
#include "android_modem_v2.h"
extern void (*android_gnssgrpcv1_send_nmea)(const char*, int);
}
#include "modem_main.h"

extern "C" {
#include "android/skin/charmap.h"
#include "hw/misc/goldfish_pstore.h"
}

#include "android-qemu2-glue/dtb.h"
#include "android-qemu2-glue/emulation/serial_line.h"
#include "android-qemu2-glue/emulation/virtio-input-multi-touch.h"
#include "android-qemu2-glue/emulation/virtio-input-rotary.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android/gps/PassiveGpsUpdater.h"
#include "android/ui-emu-agent.h"

#include <iostream>

#ifdef TARGET_AARCH64
#define TARGET_ARM64
#endif
#ifdef TARGET_I386
#define TARGET_X86
#endif

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <algorithm>
#include <string>
#include <thread>

#ifdef __APPLE__
#include <sys/resource.h>
#endif

#include "android/version.h"
#define D(...)                   \
    do {                         \
        if (VERBOSE_CHECK(init)) \
            dprint(__VA_ARGS__); \
    } while (0)

extern "C" bool android_op_wipe_data;
extern "C" bool android_op_writable_system;
extern int start_android_gnss_grpc_detached(bool& isIpv4,
                                            std::string grpcport,
                                            std::string gnssfilepath);

// Check if we are running multiple emulators on the same AVD
static bool is_multi_instance = false;

using namespace android::base;
using android::base::System;
using android::emulation::PassiveGpsUpdater;
namespace fc = android::featurecontrol;

#ifdef __linux__
static bool is_linux = true;
#else
static bool is_linux = false;
#endif

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
#if defined(__aarch64__)
#ifdef __APPLE__
        "cortex-a53",
#else   // __APPLE__
        "host",
#endif  // !__APPLE__
#else
        "cortex-a57",
#endif
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
        auto file =
                makeCustomScopedPtr(android_fopen(coreHwIniPath, "rt"), fclose);
        if (file.get() == NULL) {
            derror("Could not open hardware configuration file: "
                   "%s",
                   coreHwIniPath);
        } else {
            LineInput* input = lineInput_newFromStdFile(file.get());
            const char* line;
            dinfo("Content of hardware configuration file:");
            while ((line = lineInput_getLine(input)) != NULL) {
                dinfo("\t%s", line);
            }
            dinfo(".");
            lineInput_free(input);
        }
    }

    return 0;
}

static void prepareDisplaySettingXml(AndroidHwConfig* hw,
                                     const char* destDirectory) {
    if (!strcmp(hw->display_settings_xml, "")) {
        return;
    }
    static const int kDirFilePerm = 02750;
    std::string guestXmlDir = PathUtils::join(destDirectory, "system");
    std::string guestXmlPath =
            PathUtils::join(guestXmlDir, "display_settings.xml");
    std::string guestDisplaySettingXmlFile =
            "display_settings_" + std::string(hw->display_settings_xml) +
            ".xml";
    std::string guestDisplaySettingXmlPath =
            PathUtils::join(guestXmlDir, guestDisplaySettingXmlFile);
    if (!path_exists(guestDisplaySettingXmlPath.c_str())) {
        dwarning("Could not locate display settings file: %s",
                 guestDisplaySettingXmlPath.c_str());
        return;
    }
    path_mkdir_if_needed(guestXmlDir.c_str(), kDirFilePerm);
    path_copy_file(guestXmlPath.c_str(), guestDisplaySettingXmlPath.c_str());
    android_chmod(guestXmlPath.c_str(), 0640);
}

static void prepareDataFolder(const char* destDirectory,
                              const char* srcDirectory) {
    // The adb_keys file permission will also be set in guest system.
    // Referencing system/core/rootdir/init.usb.rc
    static const int kAdbKeyDirFilePerm = 02750;
    path_copy_dir(destDirectory, srcDirectory);
    std::string adbKeyPubPath = getAdbKeyPath(kPublicKeyFileName);
    std::string adbKeyPrivPath = getAdbKeyPath(kPrivateKeyFileName);

    if (adbKeyPubPath == "" && adbKeyPrivPath == "") {
        std::string path = PathUtils::join(
                android::ConfigDirs::getUserDirectory(), kPrivateKeyFileName);
        // try to generate the private key
        if (!adb_auth_keygen(path.c_str())) {
            dwarning("adbkey generation failed");
            return;
        }
        adbKeyPrivPath = getAdbKeyPath(kPrivateKeyFileName);
        if (adbKeyPrivPath == "") {
            return;
        }
    }
    std::string guestAdbKeyDir = PathUtils::join(destDirectory, "misc", "adb");
    std::string guestAdbKeyPath = PathUtils::join(guestAdbKeyDir, "adb_keys");

    path_mkdir_if_needed(guestAdbKeyDir.c_str(), kAdbKeyDirFilePerm);
    if (adbKeyPubPath == "") {
        // generate from private key
        std::string pubKey;
        if (pubkey_from_privkey(adbKeyPrivPath, &pubKey)) {
            FILE* pubKeyFile = fopen(guestAdbKeyPath.c_str(), "w");
            fprintf(pubKeyFile, "%s", pubKey.c_str());
            fclose(pubKeyFile);
            D("Fall back to adbkey %s successfully", adbKeyPrivPath.c_str());
        }
    } else {
        path_copy_file(guestAdbKeyPath.c_str(), adbKeyPubPath.c_str());
    }
    android_chmod(guestAdbKeyPath.c_str(), 0640);
}

static bool creatUserDataExt4Img(AndroidHwConfig* hw,
                                 const char* dataDirectory) {
    std::string empty_data_path =
            PathUtils::join(dataDirectory, "empty_data_disk");
    const bool shouldUseEmptyDataImg = path_exists(empty_data_path.c_str());
    if (shouldUseEmptyDataImg) {
        android_createEmptyExt4Image(
                hw->disk_dataPartition_path,
                getConsoleAgents()->settings->hw()->disk_dataPartition_size,
                "data");
    } else {
        android_createExt4ImageFromDir(
                hw->disk_dataPartition_path, dataDirectory,
                getConsoleAgents()->settings->hw()->disk_dataPartition_size,
                "data");
    }
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
        prepareDisplaySettingXml(hw, dataPath);

        needCopyDataPartition = !creatUserDataExt4Img(hw, dataPath);
        path_delete_dir(dataPath);
    }

    if (needCopyDataPartition) {
        if (path_exists(hw->disk_dataPartition_initPath)) {
            D("Creating: %s by copying from %s ", hw->disk_dataPartition_path,
              hw->disk_dataPartition_initPath);

            if (path_copy_file(hw->disk_dataPartition_path,
                               hw->disk_dataPartition_initPath) < 0) {
                derror("Could not create %s: %s", hw->disk_dataPartition_path,
                       strerror(errno));
                return 1;
            }

            if (!hw->hw_arc) {
                resizeExt4Partition(getConsoleAgents()
                                            ->settings->hw()
                                            ->disk_dataPartition_path,
                                    getConsoleAgents()
                                            ->settings->hw()
                                            ->disk_dataPartition_size);
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
                    driveParam +=
                            StringFormat("index=%d,id=system,if=none,file=%s",
                                         m_driveIndex++, filePath);
                } else {
                    qCow2Format = false;
                    filePath = sysImagePath.c_str();
                    driveParam += StringFormat(
                            "index=%d,id=system,if=none,file=%s"
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
                    driveParam +=
                            StringFormat("index=%d,id=vendor,if=none,file=%s",
                                         m_driveIndex++, filePath);
                } else {
                    qCow2Format = false;
                    filePath = vendorImagePath.c_str();
                    driveParam += StringFormat(
                            "index=%d,id=vendor,if=none,file=%s"
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
                        StringFormat("index=%d,id=cache,if=none,file=%s",
                                     m_driveIndex++, bufferString.c_str());
                deviceParam = StringFormat("%s,drive=cache",
                                           kTarget.storageDeviceType);
                break;
            case IMAGE_TYPE_USER_DATA:
                filePath = m_hw->disk_dataPartition_path;
                bufferString = StringFormat("%s.qcow2", filePath);
                driveParam +=
                        StringFormat("index=%d,id=userdata,if=none,file=%s",
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
                            StringFormat("index=%d,id=sdcard,if=none,file=%s",
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
                            StringFormat("index=%d,id=encrypt,if=none,file=%s",
                                         m_driveIndex++, bufferString.c_str());
                    deviceParam = StringFormat("%s,drive=encrypt",
                                               kTarget.storageDeviceType);
                } else {
                    /* no encryption partition is defined */
                    return {};
                }
                break;
            default:
                dwarning("Unknown Image type %d", type);
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

static void initialize_virtio_input_devs(android::ParameterList& args,
                                         AndroidHwConfig* hw) {
    if (fc::isEnabled(fc::VirtioInput)) {
        if (fc::isEnabled(fc::VirtioMouse)) {
            args.add("-device");
            args.add("virtio-mouse-pci");
        } else if (fc::isEnabled(fc::VirtioTablet)) {
            args.add("-device");
            args.add("virtio-tablet-pci");
        } else if (androidHwConfig_isScreenMultiTouch(hw)) {
            for (int id = 1; id <= VIRTIO_INPUT_MAX_NUM; id++) {
                args.add("-device");
                args.add(StringFormat("virtio_input_multi_touch_pci_%d", id)
                                 .c_str());
            }
        }

        if (hw->hw_rotaryInput ||
            (getConsoleAgents()->settings->avdInfo() &&
             avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
                     AVD_WEAR)) {
            args.add("-device");
            args.add("virtio_input_rotary_pci");
        }

        if (hw->hw_keyboard) {
            args.add("-device");
            args.add("virtio-keyboard-pci");
        }
    }
}

static void enableSignalTermination() {
    // The issue only occurs on Darwin so to be safe just do this on Darwin
    // to prevent potential issues. The function exists on all platforms to
    // make the calling code look cleaner.
#ifdef __APPLE__
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);

    // We will not get crash reports without the signals below enabled.
    sigaddset(&set, SIGSEGV);
    sigaddset(&set, SIGABRT);
    sigaddset(&set, SIGILL);
    int result = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    if (result != 0) {
        D("Could not set thread sigmask: %d", result);
    }
#endif
}

static bool isCrostini() {
    struct stat buf;
    return is_linux && stat("/dev/.cros_milestone", &buf) == 0;
}

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
    D("Android qemu version %s (CL:%s)",
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
#ifdef CONFIG_HEADLESS
    skin_winsys_quit_request();
#endif
}

#ifdef CONFIG_HEADLESS
#else
#if defined(_WIN32) || defined(_MSC_VER)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that later calls qMain(). In the MSVC build, qtmain.lib calls
// main() instead of qMain(), so we need to make sure qMain is redefined to
// main for that case.
#define main qt_main
#endif  // windows
#endif  // !CONFIG_HEADLESS

// create the modem_simulator sub folder on the host.
// the radio configs come from the image/<arch>/data/misc/modem_simulator
// folder, always make a copy of that folder to avd/modem_simulator/ and create
// the necessary dir strucutres that modem simulator expects.
static bool create_modem_simulator_configs(AndroidHwConfig* hw,
                                           const char* opticcprofile) {
    ScopedCPtr<char> avd_dir(path_dirname(hw->disk_dataPartition_path));
    if (!avd_dir) {
        return false;
    }
    std::string modem_config_dir =
            PathUtils::join(avd_dir.get(), "modem_simulator");
    std::string iccprofile_path = PathUtils::join(modem_config_dir.c_str(),
                                                  "iccprofile_for_sim0.xml");
    if (android_op_wipe_data) {
        path_delete_dir(modem_config_dir.c_str());
    }

    ScopedCPtr<char> sysimg_dir(path_dirname(hw->disk_ramdisk_path));
    std::string org_modem_config_dir = PathUtils::join(
            sysimg_dir.get(), "data", "misc", "modem_simulator");
    // Bug: 214140573 Unable to sign into Google Apps since GmsCore v21.42.18
    // Always make a new copy so that the timestamp of modem_simulator will be
    // newer than userdata-qemu.img. Otherwise, Google auth would fail.
    path_copy_dir(modem_config_dir.c_str(), org_modem_config_dir.c_str());
    const bool need_overwrite_iccprofile =
            opticcprofile && path_exists(opticcprofile);
    if (need_overwrite_iccprofile) {
        // need to overwite as the copy dir will copy everything
        path_copy_file(iccprofile_path.c_str(), opticcprofile);
    }
    return path_exists(iccprofile_path.c_str());
}

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

bool handleCpuAccelerationForMinConfig(int argc,
                                       char** argv,
                                       CpuAccelMode* accel_mode,
                                       char** accel_status) {
    dprint("%s: configure CPU acceleration", __func__);

    int hasEnableKvm = 0;
    int hasEnableHax = 0;
    int hasEnableHvf = 0;
    int hasEnableWhpx = 0;
    int hasEnableGvm = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-enable-kvm")) {
            hasEnableKvm = 1;
        } else if (!strcmp(argv[i], "-enable-hax")) {
            hasEnableHax = 1;
        } else if (!strcmp(argv[i], "-enable-hvf")) {
            hasEnableHvf = 1;
        } else if (!strcmp(argv[i], "-enable-whpx")) {
            hasEnableWhpx = 1;
        } else if (!strcmp(argv[i], "-enable-gvm")) {
            hasEnableGvm = 1;
        }
    }

    int totalEnabled = hasEnableKvm + hasEnableHax + hasEnableHvf +
                       hasEnableWhpx + hasEnableGvm;

    if (totalEnabled > 1) {
        derror("%s: tried to enable more than once acceleration mode. "
               "Attempted enables: kvm %d haxm %d hvf %d whpx %d gvm %d",
               __func__, hasEnableKvm, hasEnableHax, hasEnableHvf,
               hasEnableWhpx, hasEnableGvm);
        exit(1);
    }

    dprint("%s: Checking CPU acceleration capability on host...", __func__);

    // Check acceleration capabilities on the host.
    AndroidCpuAcceleration accel_capability =
            androidCpuAcceleration_getStatus(accel_status);
    bool accel_ok = (accel_capability == ANDROID_CPU_ACCELERATION_READY);

    // Print the current status.
    if (accel_ok) {
        dprint("%s: Host can use CPU acceleration", __func__);
    } else {
        dprint("%s: Host cannot use CPU acceleration", __func__);
    }

    dprint("%s: Host CPU acceleration capability status string: [%s]", __func__,
           *accel_status);

    if (0 == totalEnabled) {
        dprint("%s: CPU acceleration disabled by user (no enabled hypervisors)",
               __func__);
        androidCpuAcceleration_resetCpuAccelerator(
                ANDROID_CPU_ACCELERATOR_NONE);
        *accel_mode = ACCEL_OFF;
        return true;
    }

    *accel_mode = ACCEL_ON;

    if (hasEnableKvm) {
        dprint("%s: Selecting KVM for CPU acceleration", __func__);
        androidCpuAcceleration_resetCpuAccelerator(ANDROID_CPU_ACCELERATOR_KVM);
    } else if (hasEnableHax) {
        dprint("%s: Selecting HAXM for CPU acceleration", __func__);
        androidCpuAcceleration_resetCpuAccelerator(ANDROID_CPU_ACCELERATOR_HAX);
    } else if (hasEnableHvf) {
        dprint("%s: Selecting HVF for CPU acceleration", __func__);
        androidCpuAcceleration_resetCpuAccelerator(ANDROID_CPU_ACCELERATOR_HVF);
    } else if (hasEnableWhpx) {
        dprint("%s: Selecting WHPX for CPU acceleration", __func__);
        androidCpuAcceleration_resetCpuAccelerator(
                ANDROID_CPU_ACCELERATOR_WHPX);
    } else if (hasEnableGvm) {
        dprint("%s: Selecting GVM for CPU acceleration", __func__);
        androidCpuAcceleration_resetCpuAccelerator(ANDROID_CPU_ACCELERATOR_GVM);
    }

    return true;
}

struct HeadLessPassiveGpsData {
    std::unique_ptr<PassiveGpsUpdater> update;
    std::string AVDconf;
    double latitude = {37.422};
    double longitude{-122.084};
    double altitude = 0;
    double velocity{0};
    double heading{0};
};

static HeadLessPassiveGpsData s_headlessGpsData;

static void process_gps_passive_data() {
    // Send a GPS location to the device
    auto sLocationAgent = getConsoleAgents()->location;
    if (sLocationAgent && sLocationAgent->gpsSendLoc) {
        // Send these to the device
        // lat 37.422 lon -122.084 al 5 vel 0 head 0
        timeval timeVal = {};
        gettimeofday(&timeVal, nullptr);

        sLocationAgent->gpsSendLoc(
                s_headlessGpsData.latitude, s_headlessGpsData.longitude,
                s_headlessGpsData.altitude, s_headlessGpsData.velocity,
                s_headlessGpsData.heading, 4, &timeVal);
    }
}

static void start_passive_gps_feeder(const std::string& AVDconfPath) {
    s_headlessGpsData.AVDconf = AVDconfPath;
    PassiveGpsUpdater::parseLocationConf(
            s_headlessGpsData.AVDconf, s_headlessGpsData.latitude,
            s_headlessGpsData.longitude, s_headlessGpsData.altitude,
            s_headlessGpsData.velocity, s_headlessGpsData.heading);
    s_headlessGpsData.update.reset(
            new PassiveGpsUpdater(process_gps_passive_data));
}

static int startEmulatorWithMinConfig(int argc,
                                      char** argv,
                                      const char* avdName,
                                      int apiLevel,
                                      const char* abi,
                                      const char* arch,
                                      bool isGoogleApis,
                                      AvdFlavor flavor,
                                      const char* gpuMode,
                                      bool noWindow,
                                      int lcdWidth,
                                      int lcdHeight,
                                      int lcdDensity,
                                      const char* lcdInitialOrientation,
                                      AndroidOptions* optsToOverride,
                                      AndroidHwConfig* hwConfigToOverride,
                                      AvdInfo** avdInfoToOverride) {
    getConsoleAgents()->settings->set_android_qemu_mode(false);
    // Min config mode and fuchsia mode are equivalent, at least for now.
    getConsoleAgents()->settings->set_min_config_qemu_mode(true);
    getConsoleAgents()->settings->set_is_fuchsia(true);

    auto opts = optsToOverride;
    auto hw = hwConfigToOverride;

    opts->avd = strdup(avdName);
    opts->gpu = strdup(gpuMode);
    opts->no_window = noWindow;

    *avdInfoToOverride = avdInfo_newCustom(avdName, apiLevel, abi, arch,
                                           isGoogleApis, flavor, opts->sysdir);
    AvdInfo* avd = *avdInfoToOverride;

    getConsoleAgents()->settings->inject_AvdInfo(avd);

    // Initialize the hw config to default values, so that code paths that
    // still rely on android_hw aren't reading uninitialized memory.
    androidHwConfig_init(hw, 0);

    str_reset(&hw->hw_initialOrientation, lcdInitialOrientation);
    hw->hw_gpu_enabled = true;
    str_reset(&hw->hw_gpu_mode, gpuMode);
    hw->hw_lcd_width = lcdWidth;
    hw->hw_lcd_height = lcdHeight;
    hw->hw_lcd_density = lcdDensity;

    // Enable RGBC light sensor for Fuchsia devices.
    hw->hw_sensors_rgbclight = true;

    auto battery = getConsoleAgents()->battery;
    if (battery && battery->setHasBattery) {
        battery->setHasBattery(false);
    }
    getConsoleAgents()->location->gpsSetPassiveUpdate(false);

    // Setup GPU acceleration. This needs to go along with user interface
    // initialization, because we need the selected backend from Qt settings.
    const UiEmuAgent uiEmuAgent = {
            getConsoleAgents()->automation,
            getConsoleAgents()->battery,
            getConsoleAgents()->cellular,
            getConsoleAgents()->clipboard,
            getConsoleAgents()->display,
            getConsoleAgents()->emu,
            getConsoleAgents()->finger,
            getConsoleAgents()->location,
            getConsoleAgents()->proxy,
            getConsoleAgents()->record,
            getConsoleAgents()->sensors,
            getConsoleAgents()->telephony,
            getConsoleAgents()->user_event,
            getConsoleAgents()->virtual_scene,
            getConsoleAgents()->car,
            getConsoleAgents()->multi_display,
            nullptr  // For now there's no uses of SettingsAgent, so we
                     //          // don't set it.
    };

    android::base::Thread::maskAllSignals();
    skin_winsys_init_args(argc, argv);
    if (!emulator_initUserInterface(opts, &uiEmuAgent)) {
        dwarning("%s: user interface init failed", __func__);
    }

    // Register the quit callback
    android::base::registerEmulatorQuitCallback([] {
        android::base::ThreadLooper::runOnMainLooper(
                [] { skin_winsys_quit_request(); });
    });

#if (SNAPSHOT_PROFILE > 1)
    dinfo("skin_winsys_init and UI finishing at uptime %" PRIu64 " ms",
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

    // BUG: 148804702: angle9 has been removed
    // BUG: 156911788: potential other way to end up with gl config failure
    if (uiPreferredGlesBackend == WINSYS_GLESBACKEND_PREFERENCE_ANGLE9) {
        skin_winsys_set_preferred_gles_backend(
                WINSYS_GLESBACKEND_PREFERENCE_ANGLE);
    }

    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;

    handleCpuAccelerationForMinConfig(argc, argv, &accel_mode, &accel_status);

    // Feature flags-related last-microsecond renderer changes
    {
        // Should enable OpenGL ES 3.x?
        if (skin_winsys_get_preferred_gles_apilevel() ==
            WINSYS_GLESAPILEVEL_PREFERENCE_MAX) {
            fc::setIfNotOverridenOrGuestDisabled(fc::GLESDynamicVersion, true);
        }

        if (skin_winsys_get_preferred_gles_apilevel() ==
            WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT) {
            fc::setEnabledOverride(fc::GLESDynamicVersion, false);
        }

        if (fc::isEnabled(fc::ForceANGLE)) {
            uiPreferredGlesBackend = skin_winsys_override_glesbackend_if_auto(
                    WINSYS_GLESBACKEND_PREFERENCE_ANGLE);
        }

        if (fc::isEnabled(fc::ForceSwiftshader)) {
            uiPreferredGlesBackend = skin_winsys_override_glesbackend_if_auto(
                    WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER);
        }
    }
    android_init_multi_display(getConsoleAgents()->emu,
                               getConsoleAgents()->record,
                               getConsoleAgents()->vm);

    RendererConfig rendererConfig;
    configAndStartRenderer(avd, opts, hw, getConsoleAgents()->vm,
                           getConsoleAgents()->emu,
                           getConsoleAgents()->multi_display,
                           uiPreferredGlesBackend, &rendererConfig);

    // Gpu configuration is set, now initialize the multi display, screen
    // recorder and screenshot callback
    bool isGuestMode =
            (!hw->hw_gpu_enabled || !strcmp(hw->hw_gpu_mode, "guest"));
    getConsoleAgents()->multi_display->setGpuMode(isGuestMode, hw->hw_lcd_width,
                                                  hw->hw_lcd_height);
    screen_recorder_init(hw->hw_lcd_width, hw->hw_lcd_height,
                         isGuestMode ? uiEmuAgent.display : nullptr,
                         getConsoleAgents()->multi_display);
    android_registerScreenshotFunc([](const char* dirname,
                                      uint32_t display) -> bool {
        return android::emulation::captureScreenshot(dirname, nullptr, display);
    });

    /* Disable the GLAsyncSwap for ANGLE so far */
    bool shouldDisableAsyncSwap =
            rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE ||
            rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE9;
    // Features to disable or enable depending on rendering backend
    // and gpu make/model/version
#if defined(__APPLE__) && defined(__aarch64__)
    shouldDisableAsyncSwap = false;
#else
    shouldDisableAsyncSwap |= !strncmp("arm", kTarget.androidArch, 3) ||
                              System::get()->getProgramBitness() == 32;
#endif
    shouldDisableAsyncSwap =
            shouldDisableAsyncSwap || async_query_host_gpu_SyncBlacklisted();

    if (shouldDisableAsyncSwap) {
        fc::setEnabledOverride(fc::GLAsyncSwap, false);
    }

    android_report_session_phase(ANDROID_SESSION_PHASE_INITGENERAL);

    // Generate a hardware-qemu.ini for this AVD.
    if (VERBOSE_CHECK(init)) {
        dprint("QEMU options list (startEmulatorWithMinConfig):");
        for (int i = 0; i < argc; i++) {
            dprint("\targv[%02d] = \"%s\"", i, argv[i]);
        }
    }

    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop, argc, argv);
    android::crashreport::CrashReporter::get()->hangDetector().pause(false);
    skin_winsys_enter_main_loop(opts->no_window);
    android::crashreport::CrashReporter::get()->hangDetector().pause(true);

    stopRenderer();
    emulator_finiUserInterface();

    cuttlefish::stop_android_modem_simulator();
    process_late_teardown();
    return 0;
}

static std::string getWriteableFilename(const char* disk_dataPartition_path,
                                        const char* filename) {
    ScopedCPtr<char> userdata_dir(path_dirname(disk_dataPartition_path));
    if (userdata_dir) {
        return PathUtils::join(userdata_dir.get(), filename);
    } else {
        return "";
    }
}

static bool isEmulatorCircular(const char* param, const char* val) {
    return strcmp(param, "ro.emulator.circular") == 0 &&
           (strncmp(val, "1", 1) == 0 || strncmp(val, "y", 1) == 0 ||
            strncmp(val, "on", 2) == 0 || strncmp(val, "yes", 3) == 0 ||
            strncmp(val, "true", 4) == 0);
}

#if defined(TARGET_X86_64) || defined(TARGET_I386)
constexpr bool targetIsX86 = true;
#else
constexpr bool targetIsX86 = false;
#endif

static std::string buildSoundhwParam(const int apiLevel,
                                     const AndroidHwConfig* hw) {
    std::string param;
    std::string props;

    if (feature_is_enabled(kFeature_VirtioSndCard)) {
        param = "virtio-snd-pci";
    } else if (apiLevel >= 26 || targetIsX86) {
        /* for those system images that don't have the virtio-snd driver yet. */
        param = "hda";
    } else {
        return "";
    }

    if (!hw->hw_audioInput) {
        props += "input=off";
    }

    if (!hw->hw_audioOutput) {
        if (!props.empty()) {
            props += ",";
        }
        props += "output=off";
    }

    if (!props.empty()) {
        param += ":";
        param += props;
    }

    return param;
}

extern "C" AndroidProxyCB* gAndroidProxyCB;
extern "C" int main(int argc, char** argv) {
    if (argc < 1) {
        derror("Invalid invocation (no program path)");
        return 1;
    }

    // Initialize crash handler
    crashhandler_init(argc, argv);
#ifdef __APPLE__
    {
        int ret;
        struct rlimit rl;
        static constexpr rlim_t kDesiredFileLimit = 16384;
        rlim_t desiredLimit = kDesiredFileLimit;
        bool raiseLimit = true;
        ret = getrlimit(RLIMIT_NOFILE, &rl);

        if (0 == ret) {
            D("Num files limit: cur max %u %u", rl.rlim_cur, rl.rlim_max);
            if (desiredLimit < rl.rlim_cur) {
                raiseLimit = false;
                D("Current limit already high enough, don't raise limit.");
            } else if (desiredLimit > rl.rlim_max) {
                desiredLimit = rl.rlim_max;
                D("Target files limit: %u", desiredLimit);
            }
        } else {
            derror("%s: Failed to query files limit. errno %d", __func__,
                   errno);
        }

        if (raiseLimit) {
            rl.rlim_cur = desiredLimit;
            ret = setrlimit(RLIMIT_NOFILE, &rl);
            if (0 == ret) {
                D("Raised open files limit to %u", __func__, desiredLimit);
            } else {
                derror("%s: Failed to raise files limit. errno %d", __func__,
                       errno);
            }
            ret = getrlimit(RLIMIT_NOFILE, &rl);
            if (0 == ret) {
                D("Num files limit (after): cur max %u %u", __func__,
                  rl.rlim_cur, rl.rlim_max);
            } else {
                derror("%s: Failed to query files limit. errno %d", __func__,
                       errno);
            }
        }
    }
#endif
    process_early_setup(argc, argv);
    android_report_session_phase(ANDROID_SESSION_PHASE_PARSEOPTIONS);
    // Start GPU information query to use it later for the renderer seleciton.
    async_query_host_gpu_start();

    const char* executable = argv[0];
    android::ParameterList args = {executable};
    AvdInfo* avd;
    AndroidOptions opts[1];
    int exitStatus = 0;

    gAndroidProxyCB->ProxySet = qemu_android_setup_http_proxy;
    gAndroidProxyCB->ProxyUnset = qemu_android_remove_http_proxy;
    qemu_android_init_http_proxy_ops();

    // Make the console agents available.
    injectQemuConsoleAgents("");

#ifdef CONFIG_HEADLESS
    getConsoleAgents()->settings->set_host_emulator_is_headless(true);
    D("emulator running in headless mode");
#else
    getConsoleAgents()->settings->set_host_emulator_is_headless(false);
    D("emulator running in qt mode");
#endif

    // ParameterList params(argc, argv);
    getConsoleAgents()->settings->inject_android_cmdLine(
            android::base::createEscapedLaunchString(argc, argv).c_str());

    AndroidHwConfig* hw = getConsoleAgents()->settings->hw();
    if (!emulator_parseCommonCommandLineOptions(&argc, &argv,
                                                kTarget.androidArch,
                                                true,  // is_qemu2
                                                opts, hw, &avd, &exitStatus)) {
        // Special case for QEMU positional parameters (or Fuchsia path)
        if (exitStatus == EMULATOR_EXIT_STATUS_POSITIONAL_QEMU_PARAMETER) {
            // Copy all QEMU options to |args|, and set |n| to the number
            // of options in |args| (|argc| must be positive here).
            // NOTE: emulator_parseCommonCommandLineOptions has side effects
            // and modifies argc, as well as argv. Because of these magical
            // side effects we are *NOT* just copying over argc, argv.

            // If running Fuchsia, the kernel argument needs to be passed
            // through as when opts->fuchsia is true, since it is not a Linux
            // kernel, we do not run it through the usual parsing scheme that
            // writes the kernel path to
            // getConsoleAgents()->settings->hw()->kernel_path (android_hw is
            // currently not used in the Fuchsia path).
            if (opts->fuchsia) {
                if (opts->kernel) {
                    args.add({"-kernel", opts->kernel});
                }
                std::string dataDir = getNthParentDir(executable, 3U);
                if (dataDir.empty()) {
                    dataDir = "lib/pc-bios";
                } else {
                    dataDir += "/lib/pc-bios";
                }

                args.add({"-L", dataDir});
                for (int n = 1; n < argc; ++n) {
                    args.add(argv[n]);
                }

                // Setup console ports if requested.
                args.add2If("-android-ports", opts->ports);
                if (opts->port) {
                    int console_port = -1;
                    int adb_port = -1;
                    if (!android_parse_port_option(opts->port, &console_port,
                                                   &adb_port)) {
                        return 1;
                    }
                    args.add("-android-ports");
                    args.addFormat("%d,%d", console_port, adb_port);
                }

                fc::setIfNotOverriden(fc::HVF, true);
                fc::setIfNotOverriden(fc::Vulkan, true);
                fc::setIfNotOverriden(fc::GLDirectMem, true);
                fc::setIfNotOverriden(fc::VirtioInput, true);
                fc::setIfNotOverriden(fc::VirtioMouse, false);
                fc::setEnabledOverride(fc::RefCountPipe, false);
                fc::setIfNotOverriden(fc::VulkanNullOptionalStrings, true);
                fc::setIfNotOverriden(fc::VulkanIgnoredHandles, true);
                fc::setIfNotOverriden(fc::NoDelayCloseColorBuffer, true);
                fc::setIfNotOverriden(fc::VulkanShaderFloat16Int8, true);
                fc::setIfNotOverriden(fc::KeycodeForwarding, true);
                fc::setIfNotOverriden(fc::VulkanQueueSubmitWithCommands, true);
                fc::setIfNotOverriden(fc::VulkanBatchedDescriptorSetUpdate,
                                      true);
                fc::setIfNotOverriden(fc::VirtioGpuFenceContexts, false);
                fc::setIfNotOverriden(fc::HostComposition, true);
                fc::setIfNotOverriden(fc::AsyncComposeSupport, true);
                fc::setIfNotOverriden(fc::VulkanAstcLdrEmulation, true);

                int lcdWidth = 1280;
                int lcdHeight = 720;
                if (opts->window_size) {
                    if (sscanf(opts->window_size, "%dx%d", &lcdWidth,
                               &lcdHeight) != 2) {
                        derror("%s: invalid window size: %s", __func__,
                               opts->window_size);
                        return -1;
                    }
                }

                // Handle input args.
                if (!emulator_parseInputCommandLineOptions(opts)) {
                    return 1;
                }

                initialize_virtio_input_devs(args, hw);
                return startEmulatorWithMinConfig(
                        args.size(), args.array(), "custom", 25,
#ifdef __aarch64__
                        "arm64-v8a", "arm64",
#else
                        "x86_64", "x86_64",
#endif
                        true, AVD_PHONE, opts->gpu ? opts->gpu : "host",
                        opts->no_window, lcdWidth, lcdHeight,
                        // LCD DPI, orientation
                        96, "landscape", opts, hw, &avd);

            } else {
                for (int n = 1; n <= argc; ++n) {
                    args.add(argv[n - 1]);
                }
            }

            for (int i = 0; i < args.size(); i++) {
                dinfo("%s: arg: %s", __func__, args[i].c_str());
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

    getConsoleAgents()->settings->inject_AvdInfo(avd);

    bool lowDisk = System::isUnderDiskPressure(avdInfo_getContentPath(avd));
    if (lowDisk) {
        derror("Not enough disk space to run AVD '%s'. Exiting...",
               avdInfo_getName(avd));
        return 1;
    }

    if (opts->read_only) {
        android::base::disableRestart();
    } else {
        android::base::finalizeEmulatorRestartParameters(
                avdInfo_getContentPath(avd));
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
               "and timeout has expired. Exiting...",
               avdInfo_getName(avd));
        return 1;
    }
    if (opts->read_only) {
        TempFile* tempIni = tempfile_create();
        coreHwIniPath = tempfile_path(tempIni);
        is_multi_instance = true;
        opts->no_snapshot_save = true;
        args.add("-read-only");
    } else if (!opts->check_snapshot_loadable &&
               filelock_create_timeout(coreHwIniPath, 2000) == NULL) {
        /* The AVD is already in use, we still support this as an
         * experimental feature. Use a temporary hardware-qemu.ini
         * file though to avoid overwriting the existing one. */
        derror("Running multiple emulators with the same AVD ");
        derror("is an experimental feature.");
        derror("Please use -read-only flag to enable this feature.");
        return 1;
    }

    android::base::FileShare shareMode =
            opts->read_only ? FileShare::Read : FileShare::Write;
    if (!android::multiinstance::initInstanceShareMode(shareMode)) {
        return 1;
    }

    if (snapshotLock) {
        filelock_release(snapshotLock);
    }

    if (!emulator_parseFeatureCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    if (!emulator_parseInputCommandLineOptions(opts)) {
        return 1;
    }

    if (!emulator_parseUiCommandLineOptions(opts, avd, hw)) {
        return 1;
    }

    if (!strcmp(hw->hw_camera_back, "virtualscene")) {
        if (!feature_is_enabled(kFeature_VirtualScene)) {
            // If the virtual scene camera is selected in the avd, but not
            // supported, use the emulated camera instead.
            str_reset(&hw->hw_camera_back, "emulated");
        } else {
            // Parse virtual scene command line options, if enabled.
            camera_virtualscene_parse_cmdline();
        }
    } else if (!strcmp(hw->hw_camera_back, "videoplayback")) {
        if (!feature_is_enabled(kFeature_VideoPlayback)) {
            // If the video playback camera is selected in the avd, but not
            // supported, use the emulated camera instead.
            str_reset(&hw->hw_camera_back, "emulated");
        }
    }

    if (opts->shared_net_id) {
        char* end;
        long shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            fprintf(stderr,
                    "option -shared-net-id must be an integer between 1 and "
                    "255\n");
            return 1;
        }
        boot_property_add_shared_net_ip(shared_net_id);
    }

#ifdef CONFIG_NAND_LIMITS
    args.add2If("-nand-limits", opts->nand_limits);
#endif

    args.add2If("-timezone", opts->timezone);
    args.add2If("-cpu-delay", opts->cpu_delay);
    args.add2If("-dns-server", opts->dns_server);
    args.addIf("-skip-adb-auth", opts->skip_adb_auth);

    if (opts->audio && !strcmp(opts->audio, "none") ||
        (!hw->hw_audioInput && !hw->hw_audioOutput)) {
        // TODO(b/161814396): Be able to disable audio input/output separately
        args.add("-no-audio");
    }

    if (opts->allow_host_audio)
        args.add("-allow-host-audio");

    if (opts->restart_when_stalled)
        args.add("-restart-when-stalled");

    bool badSnapshots = false;

    // Check situations where snapshots should be turned off
    {
        // Just dont' use snapshots on 32 bit - crashes galore
        badSnapshots =
                badSnapshots || (System::get()->getProgramBitness() == 32);

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
            feature_set_enabled_override(kFeature_GenericSnapshotsUI, false);
            feature_set_enabled_override(kFeature_QuickbootFileBacked, false);
        }

        // Situations where not to use mmap() for RAM
        // 1. Using HDD on Linux or macOS; no file mapping or we will have a bad
        // time.
        // 2. macOS when having a machine with < 8 logical cores
        // 3. Apple Silicon (we probably messed up some file page size or
        // synchronization somewhere)
        if (avd) {
            auto contentPath = avdInfo_getContentPath(avd);
            auto diskKind = System::get()->pathDiskKind(contentPath);
            if (diskKind) {
                if (*diskKind == System::DiskKind::Hdd) {
                    androidSnapshot_setUsingHdd(true /* is hdd */);
#ifndef _WIN32
                    feature_set_if_not_overridden(kFeature_QuickbootFileBacked,
                                                  false /* enable */);
#endif
                }
            }
#ifdef __APPLE__
            auto numCores = System::get()->getCpuCoreCount();
            if (numCores < 8) {
                feature_set_if_not_overridden(kFeature_QuickbootFileBacked,
                                              false /* enable */);
            }
#ifdef __aarch64__
            // TODO: Fix file-backed RAM snapshot support.
            feature_set_if_not_overridden(kFeature_QuickbootFileBacked,
                                          false /* enable */);
#endif
#endif

            if (isCrostini()) {
                feature_set_if_not_overridden(kFeature_QuickbootFileBacked,
                                              false /* enable */);
            }
        }
        // 2. TODO

        // If we are changing the language, country, or locale, do not load
        // snapshot.
        if (opts->change_language || opts->change_country ||
            opts->change_locale) {
            getConsoleAgents()->settings->inject_language(opts->change_language,
                                                          opts->change_country,
                                                          opts->change_locale);

            opts->no_snapshot_load = true;
        }
    }

    if (opts->snapshot && feature_is_enabled(kFeature_FastSnapshotV1)) {
        if (!opts->no_snapshot_load) {
            args.add2("-loadvm", opts->snapshot);
        }
    }

    bool useQuickbootRamFile =
            feature_is_enabled(kFeature_QuickbootFileBacked) && !opts->snapshot;

    if (useQuickbootRamFile) {
        ScopedCPtr<const char> memPath(
                androidSnapshot_prepareAutosave(hw->hw_ramSize, nullptr));

        if (memPath) {
            args.add2("-mem-path", memPath.get());

            bool mapAsShared = !opts->read_only && !opts->snapshot &&
                               !opts->no_snapshot_save &&
                               !opts->check_snapshot_loadable &&
                               androidSnapshot_getQuickbootChoice();

            if (mapAsShared) {
                args.add("-mem-file-shared");
                androidSnapshot_setRamFileDirty(nullptr, true);
            }
        } else {
            dwarning(
                    "could not initialize Quickboot RAM file. "
                    "Please ensure enough disk space for the guest RAM size "
                    "(%d MB) along with a safety factor.",
                    hw->hw_ramSize);
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

    // Always setup a single serial port, that can be connected
    // either to the 'null' chardev, or the -shell-serial one,
    // which by default will be either 'stdout' (Posix) or 'con:'
    // (Windows).
    const char* const serial =
            (!opts->virtio_console && (opts->shell || opts->show_kernel))
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
            hw->hw_lcd_circular = isEmulatorCircular(iter->name, iter->value);
        }
    }

    for (const ParamList* pl = opts->prop; pl != NULL; pl = pl->next) {
        if (strncmp(pl->param, "qemu.", 5)) {
            dwarning(
                    "unexpected '-prop' value ('%s'), only "
                    "'qemu.*' properties are supported",
                    pl->param);
        }
        args.add2("-boot-property", pl->param);
    }

    args.add2If("-android-wifi-client-port", opts->wifi_client_port);
    args.add2If("-android-wifi-server-port", opts->wifi_server_port);

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
    std::string dataPath = PathUtils::join(
            avdInfo_getContentPath(avd),
            avdInfo_inAndroidBuild(avd) ? "build.avd/data" : "data");
    std::string pstorePath = PathUtils::join(dataPath, "misc", "pstore");
    std::string pstoreFile = PathUtils::join(pstorePath, "pstore.bin");
    if (android_op_wipe_data) {
        path_delete_file(pstoreFile.c_str());
    }
    path_mkdir_if_needed(pstorePath.c_str(), 0777);
    android_chmod(pstorePath.c_str(), 0777);

    // TODO(jansene): pstore conflicts with memory maps on Apple Silicon
#if defined(__APPLE__) && defined(__aarch64__)
    mem_map pstore = {.start = 0, .size = 0};
#else
    mem_map pstore = {.start = GOLDFISH_PSTORE_MEM_BASE,
                      .size = GOLDFISH_PSTORE_MEM_SIZE};

    args.add("-device");
    args.addFormat("goldfish_pstore,addr=0x%" PRIx64 ",size=0x%" PRIx64
                   ",file=%s",
                   pstore.start, pstore.size, pstoreFile.c_str());

#endif
    bool firstTimeSetup =
            (android_op_wipe_data || !path_exists(hw->disk_dataPartition_path));

    // studio avd manager does not allow user to change partition size, set a
    // lower limit to 6GB.
    constexpr auto kMinPlaystoreImageSize = 6LL * 1024 * 1024 * 1024;
    const int myApiLevel = avd ? avdInfo_getApiLevel(avd) : 1000;
    if (myApiLevel >= 24 || fc::isEnabled(fc::PlayStoreImage)) {
        if (firstTimeSetup &&
            getConsoleAgents()->settings->hw()->disk_dataPartition_size <
                    kMinPlaystoreImageSize) {
            getConsoleAgents()->settings->hw()->disk_dataPartition_size =
                    kMinPlaystoreImageSize;
            // Write it to config.ini as well, or we get all sorts of problems.
            if (getConsoleAgents()->settings->avdInfo()) {
                avdInfo_replaceDataPartitionSizeInConfigIni(
                        getConsoleAgents()->settings->avdInfo(),
                        kMinPlaystoreImageSize);
            }
        }
    }

    // Create userdata file from init version if needed.
    if (firstTimeSetup) {
        // Check free space first if the path does not exist.
        if (!path_exists(hw->disk_dataPartition_path)) {
            System::FileSize availableSpace;

            auto dataPartitionPathAsDir =
                    PathUtils::pathToDir(hw->disk_dataPartition_path);

            if (dataPartitionPathAsDir &&
                System::get()->pathFreeSpace(*dataPartitionPathAsDir,
                                             &availableSpace)) {
                constexpr double kDataPartitionSafetyFactor = 1.2;

                double needed = kDataPartitionSafetyFactor *
                                getConsoleAgents()
                                        ->settings->hw()
                                        ->disk_dataPartition_size /
                                (1024.0 * 1024.0);

                double available = (double)availableSpace / (1024.0 * 1024.0);

                if (needed > available) {
                    derror("Not enough space to create userdata partition. "
                           "Available: %f MB at %s, "
                           "need %f MB.",
                           available, dataPartitionPathAsDir->c_str(), needed);
                    return 1;
                }
            } else {
                // default to assuming enough space if the free space query
                // fails.
            }
        }

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
                    getConsoleAgents()
                            ->settings->hw()
                            ->disk_dataPartition_size);
            if (getConsoleAgents()->settings->hw()->disk_dataPartition_size >
                        0 &&
                current_data_size < partition_size) {
                dwarning(
                        "userdata partition is resized from %d M to %d "
                        "M",
                        (int)(current_data_size / (1024 * 1024)),
                        (int)(partition_size / (1024 * 1024)));
                if (!resizeExt4Partition(getConsoleAgents()
                                                 ->settings->hw()
                                                 ->disk_dataPartition_path,
                                         getConsoleAgents()
                                                 ->settings->hw()
                                                 ->disk_dataPartition_size)) {
                    path_delete_file(
                            StringFormat("%s.qcow2",
                                         getConsoleAgents()
                                                 ->settings->hw()
                                                 ->disk_dataPartition_path)
                                    .c_str());
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

#ifdef _WIN32
    if ((accelerator == ANDROID_CPU_ACCELERATOR_HAX ||
         accelerator == ANDROID_CPU_ACCELERATOR_GVM) &&
        ::android::base::Win32Utils::getServiceStatus("vgk") > SVC_NOT_FOUND) {
        dwarning(
                "Vanguard anti-cheat software is deteced on your system. "
                "It is known to have compatibility issues with Android "
                "emulator. It is recommended to uninstall or deactivate "
                "Vanguard anti-cheat software while running Android "
                "emulator.");
    }
#endif  // _WIN32
#else   // !TARGET_X86_64 && !TARGET_I386
#if defined(__aarch64__)
    args.add2("-machine", "type=virt");
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    const bool accel_ok =
            handleCpuAcceleration(opts, avd, &accel_mode, &accel_status);
    if (accel_ok) {
#ifdef __APPLE__
        if (avd && avdInfo_getApiLevel(avd) >= 21 &&
            !strcmp(avdInfo_getTargetAbi(avd), "arm64-v8a")) {
            args.add("-enable-hvf");
        } else {
            dwarning("hvf is not enabled on this aarch64 host.");
        }
#else
        args.add("-enable-kvm");
#endif
        if (hw->hw_cpu_ncore > 1) {
            args.add("-smp");
            args.addFormat("cores=%d", hw->hw_cpu_ncore);
        }
    } else {
        dwarning("kvm is not enabled on this aarch64 host.");
    }
#else
    args.add2("-machine", "type=ranchu");
#endif
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
        if (hw->hw_cpu_ncore > 16 &&
            accelerator == ANDROID_CPU_ACCELERATOR_HAX) {
            dwarning(
                    "HAXM does not support more than 16 cores. Number of cores "
                    "set to 16");
            hw->hw_cpu_ncore = 16;
        }
#endif
        args.addFormat("cores=%d", hw->hw_cpu_ncore);

#ifdef __APPLE__
        // macOS emulator is super slow on machines with less
        // than 6 logical cores.
        if (System::get()->getCpuCoreCount() < 6) {
            dwarning(
                    "Running on a system with less than 6 logical cores. "
                    "Setting number of virtual cores to 1");
            hw->hw_cpu_ncore = 1;
        }
#endif
    }
#endif  // !TARGET_X86_64 && !TARGET_I386

    // Memory size
    args.add("-m");
    args.addFormat("%d", hw->hw_ramSize);

    const int apiLevel = avd ? avdInfo_getApiLevel(avd) : 1000;
    if (apiLevel < 30) {
        // We have an unfortunate bug (b/189970804) in gralloc in QT in the
        // shared slots host memory allocator and it is too late to fix
        // it there.
        fc::setIfNotOverriden(fc::HasSharedSlotsHostMemoryAllocator, false);
    }

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

    std::string bootconfigInitrdPath;

    if (hw->hw_arc) {
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
    } else {
        if (hw->disk_ramdisk_path) {
            args.add2("-kernel", hw->kernel_path);

            if (fc::isEnabled(fc::AndroidbootProps) ||
                fc::isEnabled(fc::AndroidbootProps2)) {
                bootconfigInitrdPath = getWriteableFilename(
                        hw->disk_dataPartition_path, "initrd");
                args.add2("-initrd", bootconfigInitrdPath.c_str());
            } else {
                args.add2("-initrd", hw->disk_ramdisk_path);
            }
        } else {
            derror("disk_ramdisk_path is required but missing");
            return 1;
        }

        // add partition parameters with the sequence pre-defined in
        // targetInfo.imagePartitionTypes
        args.add(PartitionParameters::create(hw, avd));
    }

    if (fc::isEnabled(fc::KernelDeviceTreeBlobSupport)) {
        const std::string dtbFileName = getWriteableFilename(
                hw->disk_dataPartition_path, "default.dtb");

        if (android_op_wipe_data || !path_exists(dtbFileName.c_str())) {
            ::dtb::Params params;

            char* vendor_path = avdInfo_getVendorImageDevicePathInGuest(avd);
            if (vendor_path) {
                params.vendor_device_location = vendor_path;
                free(vendor_path);

                exitStatus = createDtbFile(params, dtbFileName);
                if (exitStatus) {
                    derror("Could not create a DTB file (%s)",
                           dtbFileName.c_str());
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
    // Configure the tap device to use vmnet
    if (opts->vmnet || opts->wifi_vmnet) {
        args.addFormat("tap,id=mynet,ifname=%s,vmnet=on",
                       opts->vmnet ? opts->vmnet : opts->wifi_vmnet);
    } else if (opts->net_tap) {
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
        if (opts->network_user_mode_options &&
            android_validate_user_mode_networking_option(
                    opts->network_user_mode_options)) {
            args.addFormat("user,id=mynet,%s", opts->network_user_mode_options);
        } else {
            args.add("user,id=mynet");
        }
    }
    args.add("-device");
    args.addFormat("%s,netdev=mynet", kTarget.networkDeviceType);

    const bool createVirtconsoles =
            opts->virtio_console || fc::isEnabled(fc::VirtconsoleLogcat);

    if (createVirtconsoles) {
        args.add("-chardev");
        args.addFormat(
                "%s,id=forhvc0",
                ((opts->virtio_console && opts->show_kernel) ? "stdio,echo=on"
                                                             : "null"));

        args.add("-chardev");
        if (fc::isEnabled(fc::VirtconsoleLogcat)) {
            if (opts->logcat) {
                if (opts->logcat_output) {
                    args.addFormat("file,id=forhvc1,path=%s",
                                   opts->logcat_output);
                } else {
                    args.add("stdio,id=forhvc1,echo=on");
                }
            } else {
                args.add("null,id=forhvc1");
            }

            if (!(fc::isEnabled(fc::AndroidbootProps) ||
                  fc::isEnabled(fc::AndroidbootProps2))) {
                // it is going to bootconfig, `androidboot.logcat`
                boot_property_add_logcat_pipe_virtconsole("*:V");
            }
        } else {
            args.add("null,id=forhvc1");
        }

        args.add2("-device", "virtio-serial-pci,ioeventfd=off");

        // the order of virtconsoles must be preserved
        args.add2("-device", "virtconsole,chardev=forhvc0");
        args.add2("-device", "virtconsole,chardev=forhvc1");

    } else {
        // no virtconsoles here

        if (!fc::isEnabled(fc::VirtconsoleLogcat)) {
            if (fc::isEnabled(fc::LogcatPipe)) {
                boot_property_add_logcat_pipe("*:V");
            }

            if (opts->logcat) {
                dwarning(
                        "Logcat tag filtering is currently disabled. "
                        "b/132840817, everything will be placed on stdout");
                // Output to std out if no file is provided.
                if (!opts->logcat_output) {
                    auto str = new std::ostream(std::cout.rdbuf());
                    android::emulation::LogcatPipe::registerStream(str);
                }
            }
        }
    }

    bool bluetooth_explicitly_disabled =
            !feature_is_enabled(kFeature_BluetoothEmulation) &&
            fc::isOverridden(fc::BluetoothEmulation);
    if ((feature_is_enabled(kFeature_BluetoothEmulation) ||
         feature_is_enabled_by_guest(kFeature_BluetoothEmulation)) &&
        !bluetooth_explicitly_disabled) {
        // Register the rootcanal device, this will activate rootcanal
        dprint("Bluetooth requested by %s",
               feature_is_enabled(kFeature_BluetoothEmulation) ? "user"
                                                               : "guest");
        args.add("-chardev");
        args.addFormat("rootcanal,id=rootcanal");
        args.add2("-device", "virtserialport,chardev=rootcanal,name=bluetooth");
    }

    if (feature_is_enabled(kFeature_ModemSimulator)) {
        if (create_modem_simulator_configs(hw, opts->icc_profile)) {
            init_modem_simulator();
            bool isIpv4 = false;
            int modem_simulator_port = 0;
            if (opts->modem_simulator_port) {
                if (!modem_simulator_parse_port_option(
                            opts->modem_simulator_port,
                            &modem_simulator_port)) {
                    return 1;
                }
            }

            // start modem now, so qemu can proceed with virtioport setup
            int modem_simulator_guest_port =
                    cuttlefish::start_android_modem_simulator_detached(
                            modem_simulator_port, isIpv4);

            args.add("-device");
            args.add("virtio-serial,ioeventfd=off");
            args.add("-chardev");
            args.addFormat(
                    "socket,port=%d,host=%s,nowait,nodelay,%s,id="
                    "modem",
                    modem_simulator_guest_port, isIpv4 ? "127.0.0.1" : "::1",
                    isIpv4 ? "ipv4" : "ipv6");
            args.add("-device");
            args.add("virtserialport,chardev=modem,name=modem");
        } else {
            dwarning(
                    "Could not setup modem simulator config files, modem "
                    "simulator disabled.");
        }
    }

    if (feature_is_enabled(kFeature_GnssGrpcV1)) {
        bool isIpv4 = false;
        // start gnss grpc now, so qemu can proceed with virtioport setup
        int gnss_guest_port = start_android_gnss_grpc_detached(
                isIpv4, opts->gnss_grpc_port ? opts->gnss_grpc_port : "",
                opts->gnss_file_path ? opts->gnss_file_path : "");

        args.add("-device");
        args.add("virtio-serial,ioeventfd=off");
        args.add("-chardev");
        args.addFormat(
                "socket,port=%d,host=%s,nowait,nodelay,%s,id="
                "gnss",
                gnss_guest_port, isIpv4 ? "127.0.0.1" : "::1",
                isIpv4 ? "ipv4" : "ipv6");
        args.add("-device");
        args.add("virtserialport,chardev=gnss,name=gnss");

        getConsoleAgents()->location->gpsEnableGnssGrpcV1();
    }

    getConsoleAgents()->location->gpsSetPassiveUpdate(!opts->no_passive_gps);

    // for headless mode, start a thread to feed guest gps hal with
    // passive gps locations; qt emulator already does that in location-page.cpp
#ifdef CONFIG_HEADLESS
    if (getConsoleAgents()->location->gpsGetPassiveUpdate()) {
        dprint("feeding guest with passive gps data, in headless mode");
        std::string AVDconfPath =
                PathUtils::join(avdInfo_getContentPath(avd), "AVDconf");
        start_passive_gps_feeder(AVDconfPath);
    }
#endif

    // rng
#if defined(TARGET_X86_64) || defined(TARGET_I386)
    args.add("-device");
    args.add("virtio-rng-pci");
#else
    args.add("-device");
    args.add("virtio-rng-device");
#endif
    args.add("-show-cursor");

    if (fc::isEnabled(fc::VirtioGpuNativeSync) ||
        !strcmp(hw->hw_gltransport, "virtio-gpu") ||
        !strcmp(hw->hw_gltransport, "virtio-gpu-pipe") ||
        !strcmp(hw->hw_gltransport, "virtio-gpu-asg")) {
        args.add("-device");
        args.add("virtio-gpu-pci");
    }
    initialize_virtio_input_devs(args, hw);
    if (feature_is_enabled(kFeature_VirtioWifi)) {
        args.add("-netdev");
        if (opts->vmnet || opts->wifi_vmnet) {
            args.addFormat("tap,id=virtio-wifi,ifname=%s,vmnet=on",
                           opts->vmnet ? opts->vmnet : opts->wifi_vmnet);
        } else if (opts->wifi_tap) {
            const char* upScript =
                    opts->wifi_tap_script_up ? opts->wifi_tap_script_up : "no";
            const char* downScript = opts->wifi_tap_script_down
                                             ? opts->wifi_tap_script_down
                                             : "no";
            args.addFormat(
                    "tap,id=virtio-wifi,script=%s,downscript=%s,ifname=%s",
                    upScript, downScript, opts->wifi_tap);
        } else {
            if (opts->wifi_tap_script_up) {
                dwarning(
                        "-wifi-tap-script-up ignored without -wifi-tap option");
            }
            if (opts->wifi_tap_script_down) {
                dwarning(
                        "-wifi-tap-script-down ignored without -wifi-tap "
                        "option");
            }
            if (opts->wifi_user_mode_options &&
                android_validate_user_mode_networking_option(
                        opts->wifi_user_mode_options)) {
                args.addFormat("user,id=virtio-wifi,%s",
                               opts->wifi_user_mode_options);
            } else {
                args.add("user,id=virtio-wifi,dhcpstart=10.0.2.16");
            }
        }

        args.add("-device");
        args.add("virtio-wifi-pci,netdev=virtio-wifi");
    }

    if (feature_is_enabled(kFeature_VirtioVsockPipe)) {
        args.add("-device");
        args.add("virtio-vsock-pci,guest-cid=77");
    }

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

    {
        const std::string soundhw = buildSoundhwParam(apiLevel, hw);
        if (!soundhw.empty()) {
            args.add2("-soundhw", soundhw.c_str());
        }
    }

// USB
#define usb_passthrough_driver "Android USB Assistant Driver"
    if (targetIsX86 && apiLevel >= 29) {
        if (opts->usb_passthrough)
            args.add2("-device", "qemu-xhci,p2=15,p3=15");

        for (const ParamList* pl = opts->usb_passthrough; pl != NULL;
             pl = pl->next) {
#ifdef _WIN32
            if (usbassist_winusb_load(pl->param)) {
                derror("Cannot load %s for USB device \"%s\". "
                       "USB pass-through might not work.",
                       usb_passthrough_driver, pl->param);
                continue;
            }
#endif
            args.add("-device");
            args.addFormat("usb-host,%s", pl->param);
        }
    }

    /* append extra qemu parameters if any */
    for (int idx = 0; kTarget.qemuExtraArgs[idx] != NULL; idx++) {
        args.add(kTarget.qemuExtraArgs[idx]);
    }

    android_report_session_phase(ANDROID_SESSION_PHASE_INITGPU);

    auto battery = getConsoleAgents()->battery;
    if (battery && battery->setHasBattery) {
        battery->setHasBattery(getConsoleAgents()->settings->hw()->hw_battery);
    }

    android_init_multi_display(getConsoleAgents()->emu,
                               getConsoleAgents()->record,
                               getConsoleAgents()->vm);

    // Setup GPU acceleration. This needs to go along with user interface
    // initialization, because we need the selected backend from Qt settings.
    const UiEmuAgent uiEmuAgent = {
            getConsoleAgents()->automation,
            getConsoleAgents()->battery,
            getConsoleAgents()->cellular,
            getConsoleAgents()->clipboard,
            getConsoleAgents()->display,
            getConsoleAgents()->emu,
            getConsoleAgents()->finger,
            getConsoleAgents()->location,
            getConsoleAgents()->proxy,
            getConsoleAgents()->record,
            getConsoleAgents()->sensors,
            getConsoleAgents()->telephony,
            getConsoleAgents()->user_event,
            getConsoleAgents()->virtual_scene,
            getConsoleAgents()->car,
            getConsoleAgents()->multi_display,
            nullptr,  // For now there's no uses of SettingsAgent, so we
                      // don't set it.
    };

    {
        qemu2_android_serialline_init();

        /* Setup SDL UI just before calling the code */
        android::base::Thread::maskAllSignals();

        /* BUG: 138377082 On Mac OS, user can't quit the program by ctrl-c after
         * all signals are blocked on QT main loop thread.
         * Thus, we explicitly remove signals SIGINT, SIGHUP and SIGTERM from
         * the blocking mask of the QT main loop thread.
         */
        enableSignalTermination();
#if (SNAPSHOT_PROFILE > 1)
        dprint("skin_winsys_init and UI starting at uptime %" PRIu64 " ms",
               get_uptime_ms());
#endif
        skin_winsys_init_args(argc, argv);
        if (!emulator_initUserInterface(opts, &uiEmuAgent)) {
            return 1;
        }

        // We have a UI, so we can ask for consent for existing crashreports
        upload_crashes();

        if (opts->ui_only) {
            // UI only. emulator_initUserInterface() is done, so we're done.
            return 0;
        }

        // Register the quit callback
        android::base::registerEmulatorQuitCallback([] {
            android::base::ThreadLooper::runOnMainLooper(
                    [] { skin_winsys_quit_request(); });
        });

#if (SNAPSHOT_PROFILE > 1)
        dprint("skin_winsys_init and UI finishing at uptime %" PRIu64 " ms",
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
            // b/147241060
            // Chrome on R requires GLAsyncSwap, otherwise it will spam logcat
            if (avdInfo_getApiLevel(avd) >= 30) {
                fc::setIfNotOverridenOrGuestDisabled(fc::GLAsyncSwap, true);
            }

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

            // In build environment, enable gles3 if possible
            if (avdInfo_inAndroidBuild(avd)) {
                fc::setIfNotOverridenOrGuestDisabled(fc::GLESDynamicVersion,
                                                     true);
            }

#ifdef __linux__
            // On Linux enable it by default.
            fc::setIfNotOverridenOrGuestDisabled(fc::GLESDynamicVersion, true);
            fc::setIfNotOverridenOrGuestDisabled(fc::GLAsyncSwap, true);
#endif

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
        configAndStartRenderer(avd, opts, hw, getConsoleAgents()->vm,
                               getConsoleAgents()->emu,
                               getConsoleAgents()->multi_display,
                               uiPreferredGlesBackend, &rendererConfig);

        // Gpu configuration is set, now initialize the multi display, screen
        // recorder and screenshot callback
        bool isGuestMode =
                (!hw->hw_gpu_enabled || !strcmp(hw->hw_gpu_mode, "guest"));
        getConsoleAgents()->multi_display->setGpuMode(
                isGuestMode, hw->hw_lcd_width, hw->hw_lcd_height);
        screen_recorder_init(hw->hw_lcd_width, hw->hw_lcd_height,
                             isGuestMode ? uiEmuAgent.display : nullptr,
                             getConsoleAgents()->multi_display);
        android_registerScreenshotFunc(
                [](const char* dirname, uint32_t display) -> bool {
                    return android::emulation::captureScreenshot(
                            dirname, nullptr, display);
                });

        /* Disable the GLAsyncSwap for ANGLE so far */
        bool shouldDisableAsyncSwap =
                rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE ||
                rendererConfig.selectedRenderer == SELECTED_RENDERER_ANGLE9;
        // Features to disable or enable depending on rendering backend
        // and gpu make/model/version
#if defined(__APPLE__) && defined(__aarch64__)
        shouldDisableAsyncSwap = false;
#else
        shouldDisableAsyncSwap |= !strncmp("arm", kTarget.androidArch, 3) ||
                                  System::get()->getProgramBitness() == 32;
#endif
        shouldDisableAsyncSwap = shouldDisableAsyncSwap ||
                                 async_query_host_gpu_SyncBlacklisted();

        if (shouldDisableAsyncSwap) {
            fc::setEnabledOverride(fc::GLAsyncSwap, false);
        }

        // Get verified boot kernel parameters, if they exist.
        // If this is not a playstore image, then -writable_system will
        // disable verified boot.
        std::vector<std::string> verified_boot_params;
        if (feature_is_enabled(kFeature_PlayStoreImage) ||
            !android_op_writable_system ||
            feature_is_enabled(kFeature_DynamicPartition)) {
            android::verifiedboot::getParametersFromFile(
                    avdInfo_getVerifiedBootParamsPath(avd),  // NULL here is OK
                    &verified_boot_params);
            if (feature_is_enabled(kFeature_DynamicPartition)) {
                std::string boot_dev("androidboot.boot_devices=");
                boot_dev.append(avdInfo_getDynamicPartitionBootDevice(avd));
                verified_boot_params.push_back(boot_dev);
            }
            if (android_op_writable_system) {
                // unlocked state
                verified_boot_params.push_back(
                        "androidboot.verifiedbootstate=orange");
            }
        }

        const char* real_console_tty_prefix = kTarget.ttyPrefix;

        if (opts->virtio_console) {
            real_console_tty_prefix = "hvc";
        }

        constexpr bool isQemu2 = true;

        std::string myserialno("EMULATOR" EMULATOR_VERSION_STRING);
        std::replace(myserialno.begin(), myserialno.end(), '.', 'X');

        std::vector<std::pair<std::string, std::string>> userspaceBootOpts =
                getUserspaceBootProperties(
                        opts, kTarget.androidArch, myserialno.c_str(), isQemu2,
                        rendererConfig.glesMode,
                        rendererConfig.bootPropOpenglesVersion, apiLevel,
                        real_console_tty_prefix, &verified_boot_params, hw);

        std::vector<std::string> kernelCmdLineUserspaceBootOpts;
        if (fc::isEnabled(fc::AndroidbootProps) ||
            fc::isEnabled(fc::AndroidbootProps2)) {
            const int r = createRamdiskWithBootconfig(
                    hw->disk_ramdisk_path, bootconfigInitrdPath.c_str(),
                    userspaceBootOpts);
            if (r) {
                derror("%s:%d Could not prepare the ramdisk with bootconfig, "
                       "error=%d src=%s dst=%s",
                       __func__, __LINE__, r, hw->disk_ramdisk_path,
                       bootconfigInitrdPath.c_str());

                return r;
            }

            kernelCmdLineUserspaceBootOpts.push_back("bootconfig");
        } else {
            for (const auto& kv : userspaceBootOpts) {
                kernelCmdLineUserspaceBootOpts.push_back(kv.first + "=" +
                                                         kv.second);
            }
        }

        std::string append_arg = emulator_getKernelParameters(
                opts, kTarget.androidArch, apiLevel, real_console_tty_prefix,
                hw->kernel_parameters, hw->kernel_path, &verified_boot_params,
                rendererConfig.glFramebufferSizeBytes, pstore, isQemu2,
                hw->hw_arc /* isCros */,
                std::move(kernelCmdLineUserspaceBootOpts));

        if (append_arg.empty()) {
            return 1;
        }

        /* append the kernel parameters after -qemu */
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
            if (!isGuestMode)
                args.add2("-display", "sdl,gl=on");
        }

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
    crashhandler_add_string(CRASH_AVD_INI, coreHwIniPath);
    if (VERBOSE_CHECK(init)) {
        dinfo("QEMU options list:");
        for (int i = 0; i < args.size(); i++) {
            dinfo("\t argv[%02d] = \"%s\"", i, args[i].c_str());
        }
        // Dump final command-line option to make debugging the core easier
        dinfo("Concatenated QEMU options: %s", args.toString().c_str());
    }
    if (fc::isEnabled(fc::VirtioWifi)) {
        auto* hostapd = android::emulation::HostapdController::getInstance();
        if (hostapd->init(VERBOSE_CHECK(wifi))) {
            hostapd->run();
        } else {
            derror("%s: Error: could not initialize hostpad event loop.",
                   __func__);
        }
    }
    if (opts->check_snapshot_loadable) {
        android_check_snapshot_loadable(opts->check_snapshot_loadable);
        stopRenderer();
        cuttlefish::stop_android_modem_simulator();
        process_late_teardown();
        return 0;
    }

    skin_winsys_spawn_thread(opts->no_window, enter_qemu_main_loop, args.size(),
                             args.array());

    android::crashreport::CrashReporter::get()->hangDetector().pause(false);
    skin_winsys_enter_main_loop(opts->no_window);
    android::crashreport::CrashReporter::get()->hangDetector().pause(true);

    stopRenderer();
    emulator_finiUserInterface();

    cuttlefish::stop_android_modem_simulator();
    process_late_teardown();
    return 0;
}
