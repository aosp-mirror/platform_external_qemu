// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/userspace-boot-properties.h"

#include "android/base/StringFormat.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/version.h"

namespace {

// Note: The ACPI _HID that follows devices/ must match the one defined in the
// ACPI tables (hw/i386/acpi_build.c)
static const char kSysfsAndroidDtDir[] =
        "/sys/bus/platform/devices/ANDR0001:00/properties/android/";
static const char kSysfsAndroidDtDirDtb[] =
        "/proc/device-tree/firmware/android/";
}  // namespace

using android::base::StringFormat;

std::vector<std::string>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const bool isQemu2,
                           const uint32_t lcd_width,
                           const uint32_t lcd_height,
                           const uint32_t lcd_vsync,
                           const AndroidGlesEmulationMode glesMode,
                           const int bootPropOpenglesVersion,
                           const int vm_heapSize,
                           const int apiLevel,
                           const char* kernelSerialPrefix,
                           const std::vector<std::string>* verifiedBootParameters,
                           const char* gltransport,
                           const uint32_t gltransport_drawFlushInterval,
                           const char* displaySettingsXml) {
    const bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");
    const bool isARMish = !strcmp(targetArch, "arm") || !strcmp(targetArch, "arm64");
    const bool hasShellConsole = opts->logcat || opts->shell;

    namespace fc = android::featurecontrol;

    std::vector<std::string> params;

    // We always force qemu=1 when running inside QEMU.
    params.push_back("qemu=1");

    params.push_back(StringFormat(
        "androidboot.hardware=%s", isQemu2 ? "ranchu" : "goldfish"));

    if (opts->guest_angle) {
        params.push_back("androidboot.hardware.egl=angle");
    }

    {
        std::string myserialno(EMULATOR_VERSION_STRING);
        std::replace(myserialno.begin(), myserialno.end(), '.', 'X');
        params.push_back(StringFormat(
            "androidboot.serialno=EMULATOR%s", myserialno.c_str()));
    }

    if (!opts->no_jni) {
        params.push_back("android.checkjni=1");
    }
    if (opts->no_boot_anim) {
        params.push_back("android.bootanim=0");
    }

    // qemu.gles is used to pass the GPU emulation mode to the guest
    // through kernel parameters. Note that the ro.opengles.version
    // boot property must also be defined for |gles > 0|, but this
    // is not handled here (see vl-android.c for QEMU1).
    {
        int gles;
        switch (glesMode) {
            case kAndroidGlesEmulationHost: gles = 1; break;
            case kAndroidGlesEmulationGuest: gles = 2; break;
            default: gles = 0;
        }
        params.push_back(StringFormat("qemu.gles=%d", gles));
    }

    // To save battery, set the screen off timeout to a high value.
    // Using int32_max here. The unit is milliseconds.
    params.push_back(StringFormat("qemu.settings.system.screen_off_timeout=2147483647")); // 596 hours

    if (isQemu2 && fc::isEnabled(fc::EncryptUserData)) {
        params.push_back(StringFormat("qemu.encrypt=1"));
    }

    // Android media profile selection
    // 1. If the SelectMediaProfileConfig is on, then select
    // <media_profile_name> if the resolution is above 1080p (1920x1080).
    if (isQemu2 && fc::isEnabled(fc::DynamicMediaProfile)) {
        if ((lcd_width > 1920 && lcd_height > 1080) ||
            (lcd_width > 1080 && lcd_height > 1920)) {
            fprintf(stderr, "Display resolution > 1080p. Using different media profile.\n");
            params.push_back(StringFormat(
                "qemu.mediaprofile.video=%s", "/data/vendor/etc/media_codecs_google_video_v2.xml"));
        }
    }

    // Set vsync rate
    params.push_back(StringFormat("qemu.vsync=%u", lcd_vsync));

    // Set gl transport props
    params.push_back(StringFormat("qemu.gltransport=%s", gltransport));
    params.push_back(StringFormat("qemu.gltransport.drawFlushInterval=%u", gltransport_drawFlushInterval));

    // OpenGL ES related setup
    // 1. Set opengles.version and set Skia as UI renderer if
    // GLESDynamicVersion = on (i.e., is a reasonably good driver)
    params.push_back(StringFormat("qemu.opengles.version=%d", bootPropOpenglesVersion));

    if (fc::isEnabled(fc::GLESDynamicVersion)) {
        params.push_back(StringFormat("qemu.uirenderer=%s", "skiagl"));
    }

    if (opts->logcat) {
        std::string param = opts->logcat;
        // Replace any space with a comma.
        std::replace(param.begin(), param.end(), ' ', ',');
        std::replace(param.begin(), param.end(), '\t', ',');
        params.push_back(StringFormat("androidboot.logcat=%s", param));
    }

    if (opts->bootchart) {
        params.push_back(StringFormat("androidboot.bootchart=%s", opts->bootchart));
    }

    if (opts->selinux) {
        params.push_back(StringFormat("androidboot.selinux=%s", opts->selinux));
    }

    if (vm_heapSize > 0) {
        params.push_back(StringFormat("qemu.dalvik.vm.heapsize=%dm", vm_heapSize));
    }

    if (opts->legacy_fake_camera) {
        params.push_back("qemu.legacy_fake_camera=1");
    }

    if (apiLevel > 29) {
        params.push_back("qemu.camera_protocol_ver=1");
    }

    const bool isDynamicPartition = fc::isEnabled(fc::DynamicPartition);
    if (isQemu2 && isX86ish && !isDynamicPartition) {
        // x86 and x86_64 platforms use an alternative Android DT directory that
        // mimics the layout of /proc/device-tree/firmware/android/
        params.push_back(StringFormat("androidboot.android_dt_dir=%s",
            (fc::isEnabled(fc::KernelDeviceTreeBlobSupport) ?
                kSysfsAndroidDtDirDtb : kSysfsAndroidDtDir)));
    }

    if (verifiedBootParameters) {
        for (const std::string& param : *verifiedBootParameters) {
            params.push_back(param);
        }
    }

    // display settings file name
    if (displaySettingsXml && displaySettingsXml[0]) {
        params.push_back(StringFormat("qemu.display.settings.xml=%s", displaySettingsXml));
    }

    if (isQemu2) {
        if (fc::isEnabled(fc::VirtioWifi)) {
            params.push_back("qemu.virtiowifi=1");
        } else if (fc::isEnabled(fc::Wifi)) {
            params.push_back("qemu.wifi=1");
        }
    }

    if (isQemu2) {
        if (hasShellConsole) {
            params.push_back(StringFormat("androidboot.console=%s0", kernelSerialPrefix));
        }

        params.push_back("android.qemud=1");
    } else {  // !isQemu2
        // Technical note: There are several important constraints when
        // setting up QEMU1 virtual ttys:
        //
        // - The first one if that for API level < 14, the system requires
        //   that the *second* virtual serial port (i.e. ttyS1) be associated
        //   with the android-qemud chardev, used to implement the legacy
        //   QEMUD protocol. Newer API levels use a pipe for this instead.
        //
        // - The second one is that the x86 and x86_64 virtual boards have
        //   a limited number of IRQs which makes it unable to setup more
        //   than two ttys at the same time.
        //
        // - Third, specifying -logcat implies -shell due to limitations in
        //   the guest system. I.e. the shell console will always receive
        //   logcat output, even if one uses -shell-serial to redirect its
        //   output to a pty or something similar.
        //
        // We thus consider the following cases:
        //
        // * For apiLevel >= 14:
        //      ttyS0 = android-kmsg (kernel messages)
        //      ttyS1 = <shell-serial> (logcat/shell).
        //
        // * For apiLevel < 14 && !x86ish:
        //      ttyS0 = android-kmsg (kernel messages)
        //      ttyS1 = android-qemud
        //      ttyS2 = <shell-serial> (logcat/shell)
        //
        // * For apiLevel < 14 && x86ish:
        //      ttyS0 = <shell-serial> (kernel messages/logcat/shell).
        //      ttyS1 = android-qemud
        //
        // Where <shell-serial> is the value of opts->shell_serial, which
        // by default will be the 'stdio' or 'con:' chardev (for Posix and
        // Windows, respectively).

        int logcatSerial = 1;
        if (apiLevel < 14) {
            params.push_back(StringFormat("android.qemud=%s1", kernelSerialPrefix));
            if (isX86ish) {
                logcatSerial = 0;
            } else {
                logcatSerial = 2;
            }
        } else {
            // The rild daemon, used for GSM emulation, checks for qemud,
            // just set it to a dummy value instead of a serial port.
            params.push_back("android.qemud=1");
        }

        if (hasShellConsole) {
            params.push_back(StringFormat(
                "androidboot.console=%s%d", kernelSerialPrefix, logcatSerial));
        }
    }

    return params;
}
