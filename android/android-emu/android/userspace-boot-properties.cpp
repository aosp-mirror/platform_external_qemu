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

#include <algorithm>
#include <string>

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

std::vector<std::pair<std::string, std::string>>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const char* serialno,
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
    const bool hasShellConsole = opts->logcat || opts->shell;

    const char* checkjniProp;
    const char* bootanimProp;
    const char* bootanimPropValue;
    const char* qemuGlesProp;
    const char* qemuScreenOffTimeoutProp;
    const char* qemuEncryptProp;
    const char* qemuMediaProfileVideoProp;
    const char* qemuVsyncProp;
    const char* qemuGltransportNameProp;
    const char* qemuDrawFlushIntervalProp;
    const char* qemuOpenglesVersionProp;
    const char* qemuUirendererProp;
    const char* dalvikVmHeapsizeProp;
    const char* qemuLegacyFakeCameraProp;
    const char* qemuCameraProtocolVerProp;
    const char* qemuDisplaySettingsXmlProp;
    const char* qemuVirtioWifiProp;
    const char* qemuWifiProp;
    const char* androidQemudProp;
    const char* qemuHwcodecAvcdecProp;
    const char* qemuHwcodecVpxdecProp;
    const char* androidbootLogcatProp;

    namespace fc = android::featurecontrol;
    if (fc::isEnabled(fc::AndroidbootProps)) {
        checkjniProp = "androidboot.dalvik.vm.checkjni";
        bootanimProp = "androidboot.debug.sf.nobootanimation";
        bootanimPropValue = "1";
        qemuGlesProp = nullptr;  // deprecated
        qemuScreenOffTimeoutProp = "androidboot.qemu.settings.system.screen_off_timeout";
        qemuEncryptProp = nullptr;  // deprecated
        qemuMediaProfileVideoProp = nullptr;  // deprecated
        qemuVsyncProp = "androidboot.qemu.vsync";
        qemuGltransportNameProp = "androidboot.qemu.gltransport.name";
        qemuDrawFlushIntervalProp = "androidboot.qemu.gltransport.drawFlushInterval";
        qemuOpenglesVersionProp = "androidboot.opengles.version";
        qemuUirendererProp = "androidboot.debug.hwui.renderer";
        dalvikVmHeapsizeProp = "androidboot.dalvik.vm.heapsize";
        qemuLegacyFakeCameraProp = "androidboot.qemu.legacy_fake_camera";
        qemuCameraProtocolVerProp = "androidboot.qemu.camera_protocol_ver";
        qemuDisplaySettingsXmlProp = nullptr;  // deprecated
        qemuVirtioWifiProp = "androidboot.qemu.virtiowifi";
        qemuWifiProp = "androidboot.qemu.wifi";
        androidQemudProp = nullptr;  // deprecated
        qemuHwcodecAvcdecProp = "androidboot.qemu.hwcodec.avcdec";
        qemuHwcodecVpxdecProp = "androidboot.qemu.hwcodec.vpxdec";
        androidbootLogcatProp = "androidboot.logcat";
    } else {
        checkjniProp = "android.checkjni";
        bootanimProp = "android.bootanim";
        bootanimPropValue = "0";
        qemuGlesProp = "qemu.gles";
        qemuScreenOffTimeoutProp = "qemu.settings.system.screen_off_timeout";
        qemuEncryptProp = "qemu.encrypt";
        qemuMediaProfileVideoProp = "qemu.mediaprofile.video";
        qemuVsyncProp = "qemu.vsync";
        qemuGltransportNameProp = "qemu.gltransport";
        qemuDrawFlushIntervalProp = "qemu.gltransport.drawFlushInterval";
        qemuOpenglesVersionProp = "qemu.opengles.version";
        qemuUirendererProp = "qemu.uirenderer";
        dalvikVmHeapsizeProp = "qemu.dalvik.vm.heapsize";
        qemuLegacyFakeCameraProp = "qemu.legacy_fake_camera";
        qemuCameraProtocolVerProp = "qemu.camera_protocol_ver";
        qemuDisplaySettingsXmlProp = "qemu.display.settings.xml";
        qemuVirtioWifiProp = "qemu.virtiowifi";
        qemuWifiProp = "qemu.wifi";
        androidQemudProp = "android.qemud";
        qemuHwcodecAvcdecProp = "qemu.hwcodec.avcdec";
        qemuHwcodecVpxdecProp = "qemu.hwcodec.vpxdec";
        androidbootLogcatProp = nullptr;
    }

    std::vector<std::pair<std::string, std::string>> params;

    // We always force qemu=1 when running inside QEMU.
    params.push_back({"qemu", "1"});

    params.push_back({"androidboot.hardware", isQemu2 ? "ranchu" : "goldfish"});

    if (opts->guest_angle) {
        params.push_back({"androidboot.hardware.egl", "angle"});
    }

    if (serialno) {
        params.push_back({"androidboot.serialno", serialno});
    }

    if (!opts->no_jni) {
        params.push_back({checkjniProp, "1"});
    }
    if (opts->no_boot_anim) {
        params.push_back({bootanimProp, bootanimPropValue});
    }

    // qemu.gles is used to pass the GPU emulation mode to the guest
    // through kernel parameters. Note that the ro.opengles.version
    // boot property must also be defined for |gles > 0|, but this
    // is not handled here (see vl-android.c for QEMU1).
    if (qemuGlesProp) {
        int gles;
        switch (glesMode) {
            case kAndroidGlesEmulationHost: gles = 1; break;
            case kAndroidGlesEmulationGuest: gles = 2; break;
            default: gles = 0;
        }
        params.push_back({qemuGlesProp, StringFormat("%d", gles)});
    }

    // To save battery, set the screen off timeout to a high value.
    // Using int32_max here. The unit is milliseconds.
    params.push_back({qemuScreenOffTimeoutProp, "2147483647"}); // 596 hours

    if (isQemu2 && fc::isEnabled(fc::EncryptUserData) && qemuEncryptProp) {
        params.push_back({qemuEncryptProp, "1"});
    }

    // Android media profile selection
    // 1. If the SelectMediaProfileConfig is on, then select
    // <media_profile_name> if the resolution is above 1080p (1920x1080).
    if (isQemu2 && fc::isEnabled(fc::DynamicMediaProfile) && qemuMediaProfileVideoProp) {
        if ((lcd_width > 1920 && lcd_height > 1080) ||
            (lcd_width > 1080 && lcd_height > 1920)) {
            fprintf(stderr, "Display resolution > 1080p. Using different media profile.\n");
            params.push_back({
                qemuMediaProfileVideoProp,
                "/data/vendor/etc/media_codecs_google_video_v2.xml"
            });
        }
    }

    // Set vsync rate
    params.push_back({qemuVsyncProp, StringFormat("%u", lcd_vsync)});

    // Set gl transport props
    params.push_back({qemuGltransportNameProp, gltransport});
    params.push_back({
        qemuDrawFlushIntervalProp,
        StringFormat("%u", gltransport_drawFlushInterval)});

    // OpenGL ES related setup
    // 1. Set opengles.version and set Skia as UI renderer if
    // GLESDynamicVersion = on (i.e., is a reasonably good driver)
    params.push_back({
        qemuOpenglesVersionProp,
        StringFormat("%d", bootPropOpenglesVersion)
    });

    if (fc::isEnabled(fc::GLESDynamicVersion)) {
        params.push_back({qemuUirendererProp, "skiagl"});
    }

    if (androidbootLogcatProp) {
        if (opts->logcat) {
            std::string param = opts->logcat;

            // Replace any space with a comma.
            std::replace_if(param.begin(), param.end(), [](char c){
                switch (c) {
                case ' ':
                case '\t':
                    return true;

                default:
                    return false;
                }
            }, ',');

            params.push_back({androidbootLogcatProp, param});
        } else {
            params.push_back({androidbootLogcatProp, "*:V"});
        }
    }

    if (opts->bootchart) {
        params.push_back({"androidboot.bootchart", opts->bootchart});
    }

    if (opts->selinux) {
        params.push_back({"androidboot.selinux", opts->selinux});
    }

    if (vm_heapSize > 0) {
        params.push_back({
            dalvikVmHeapsizeProp,
            StringFormat("%dm", vm_heapSize)
        });
    }

    if (opts->legacy_fake_camera) {
        params.push_back({qemuLegacyFakeCameraProp, "1"});
    }

    if (apiLevel > 29) {
        params.push_back({qemuCameraProtocolVerProp, "1"});
    }

    const bool isDynamicPartition = fc::isEnabled(fc::DynamicPartition);
    if (isQemu2 && isX86ish && !isDynamicPartition) {
        // x86 and x86_64 platforms use an alternative Android DT directory that
        // mimics the layout of /proc/device-tree/firmware/android/
        params.push_back({
            "androidboot.android_dt_dir",
            (fc::isEnabled(fc::KernelDeviceTreeBlobSupport) ?
                kSysfsAndroidDtDirDtb : kSysfsAndroidDtDir)
            });
    }

    if (verifiedBootParameters) {
        for (const std::string& param : *verifiedBootParameters) {
            const size_t i = param.find('=');
            if (i == std::string::npos) {
                params.push_back({param, ""});
            } else {
                params.push_back({param.substr(0, i), param.substr(i + 1)});
            }
        }
    }

    // display settings file name
    if (displaySettingsXml && displaySettingsXml[0] && qemuDisplaySettingsXmlProp) {
        params.push_back({qemuDisplaySettingsXmlProp, displaySettingsXml});
    }

    if (isQemu2) {
        if (fc::isEnabled(fc::VirtioWifi)) {
            params.push_back({qemuVirtioWifiProp, "1"});
        } else if (fc::isEnabled(fc::Wifi)) {
            params.push_back({qemuWifiProp, "1"});
        }
    }

    if (fc::isEnabled(fc::HardwareDecoder)) {
        params.push_back({qemuHwcodecAvcdecProp, "2"});
        params.push_back({qemuHwcodecVpxdecProp, "2"});
    }

    if (isQemu2) {
        if (hasShellConsole) {
            params.push_back({
                "androidboot.console",
                StringFormat("%s0", kernelSerialPrefix)
            });
        }

        if (androidQemudProp) {
            params.push_back({androidQemudProp, "1"});
        }
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
            if (androidQemudProp) {
                params.push_back({androidQemudProp, StringFormat("%s1", kernelSerialPrefix)});
            }

            if (isX86ish) {
                logcatSerial = 0;
            } else {
                logcatSerial = 2;
            }
        } else {
            // The rild daemon, used for GSM emulation, checks for qemud,
            // just set it to a dummy value instead of a serial port.
            if (androidQemudProp) {
                params.push_back({androidQemudProp, "1"});
            }
        }

        if (hasShellConsole) {
            params.push_back({
                "androidboot.console",
                StringFormat("%s%d", kernelSerialPrefix, logcatSerial)
            });
        }
    }

    return params;
}
