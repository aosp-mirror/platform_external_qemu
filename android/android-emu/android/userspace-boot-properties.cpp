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

#include <string.h>                                 // for strcmp, strchr
#include <algorithm>                                // for replace_if
#include <ostream>                                  // for operator<<, ostream
#include <string>                                   // for string, operator+

#include "android/avd/info.h"
#include "android/base/Log.h"                       // for LOG, LogMessage
#include "android/base/StringFormat.h"              // for StringFormat
#include "android/base/misc/StringUtils.h"          // for splitTokens
#include "android/emulation/control/adb/adbkey.h"   // for getPrivateAdbKeyPath
#include "android/emulation/resizable_display_config.h"
#include "android/featurecontrol/FeatureControl.h"  // for isEnabled
#include "android/featurecontrol/Features.h"        // for AndroidbootProps2
#include "android/hw-sensors.h"                     // for android_foldable_...
#include "android/utils/debug.h"                    // for dwarning
#include "android/utils/log_severity.h"             // for EMULATOR_LOG_ERROR

namespace {

// Note: The ACPI _HID that follows devices/ must match the one defined in the
// ACPI tables (hw/i386/acpi_build.c)
static const char kSysfsAndroidDtDir[] =
        "/sys/bus/platform/devices/ANDR0001:00/properties/android/";
static const char kSysfsAndroidDtDirDtb[] =
        "/proc/device-tree/firmware/android/";

}  // namespace

using android::base::StringFormat;
using android::base::splitTokens;

std::string getDeviceStateString(const AndroidHwConfig* hw) {
    if (android_foldable_hinge_configured()) {
        int numHinges = hw->hw_sensor_hinge_count;
        if (numHinges < 0 || numHinges > ANDROID_FOLDABLE_MAX_HINGES) {
            LOG(ERROR) << "Incorrect hinge count " <<hw->hw_sensor_hinge_count;
            return std::string();
        }
        std::string postureList(hw->hw_sensor_posture_list ? hw->hw_sensor_posture_list : "");
        std::string postureValues(hw->hw_sensor_hinge_angles_posture_definitions ? hw->hw_sensor_hinge_angles_posture_definitions : "");
        std::vector<std::string> postureListTokens, postureValuesTokens;
        splitTokens(postureList, &postureListTokens, ",");
        splitTokens(postureValues, &postureValuesTokens, ",");
        if (postureList.empty() || postureValues.empty() ||
            postureListTokens.size() != postureValuesTokens.size()) {
            LOG(ERROR) << "Incorrect posture list " << postureList
                       << " or posture mapping " << postureValues;
            return std::string();
        }
        int foldAtPosture = hw->hw_sensor_hinge_fold_to_displayRegion_0_1_at_posture;
        std::string ret("<device-state-config>");
        std::vector<std::string> valuesToken;
        for (int i = 0; i < postureListTokens.size(); i++) {
            char name[16];
            ret += "<device-state>";
            if (foldAtPosture != 1 && postureListTokens[i] == std::to_string(foldAtPosture)) {
                // "device/generic/goldfish/overlay/frameworks/base/core/res/res/values/config.xml"
                // specified "config_foldedDeviceStates" as "1" (CLOSED).
                // If foldablbe AVD configs "fold" at other deviceState, rewrite it to "1"
                postureListTokens[i] = "1";
            }
            ret += "<identifier>" + postureListTokens[i] + "</identifier>";
            if (!android_foldable_posture_name(std::stoi(postureListTokens[i], nullptr), name)) {
                return std::string();
            }
            ret += "<name>" + std::string(name) + "</name>";
            ret += "<conditions>";
            splitTokens(postureValuesTokens[i], &valuesToken,
                                       "&");
            if ( valuesToken.size() != numHinges) {
                LOG(ERROR) << "Incorrect posture mapping " << postureValuesTokens[i];
                return std::string();
            }
            std::vector<std::string> values;
            struct AnglesToPosture valuesToPosture;
            for (int j = 0; j < valuesToken.size(); j++) {
                ret += "<sensor>";
                ret += "<type>android.sensor.hinge_angle</type>";
                ret += "<name>Goldfish hinge sensor" + std::to_string(j) + " (in degrees)</name>";
                splitTokens(valuesToken[j], &values, "-");
                size_t tokenCount = values.size();
                if (tokenCount != 2 && tokenCount != 3) {
                    LOG(ERROR) << "Incorrect posture mapping " << valuesToken[j];
                    return std::string();
                }
                ret += "<value>";
                ret += "<min-inclusive>" + values[0] + "</min-inclusive>";
                ret += "<max-inclusive>" + values[1] + "</max-inclusive>";
                ret += "</value>";
                ret += "</sensor>";
            }
            ret += "</conditions>";
            ret += "</device-state>";
        }
        ret += "</device-state-config>";
        return ret;
    }

    if (android_foldable_rollable_configured()) {
        return std::string("<device-state-config><device-state><identifier>1</identifier>"
                           "<name>CLOSED</name><conditions><lid-switch><open>false</open>"
                           "</lid-switch></conditions></device-state><device-state>"
                           "<identifier>3</identifier><name>OPENED</name><conditions>"
                           "<lid-switch><open>true</open></lid-switch></conditions>"
                           "</device-state></device-state-config>");
    }

    return std::string();
}

std::vector<std::pair<std::string, std::string>>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const char* serialno,
                           const bool isQemu2,
                           const AndroidGlesEmulationMode glesMode,
                           const int bootPropOpenglesVersion,
                           const int apiLevel,
                           const char* kernelSerialPrefix,
                           const std::vector<std::string>* verifiedBootParameters,
                           const AndroidHwConfig* hw) {
    const bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");
    const bool hasShellConsole = opts->logcat || opts->shell;

    const char* androidbootVerityMode;
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
    const char* qemuCameraHqEdgeProp;
    const char* qemuDisplaySettingsXmlProp;
    const char* qemuVirtioWifiProp;
    const char* qemuWifiProp;
    const char* androidQemudProp;
    const char* qemuHwcodecAvcdecProp;
    const char* qemuHwcodecVpxdecProp;
    const char* androidbootLogcatProp;
    const char* adbKeyProp;
    const char* avdNameProp;
    const char* deviceStateProp;
    const char* qemuCpuVulkanVersionProp;

    namespace fc = android::featurecontrol;
    if (fc::isEnabled(fc::AndroidbootProps) || fc::isEnabled(fc::AndroidbootProps2)) {
        androidbootVerityMode = "androidboot.veritymode";
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
        qemuCameraHqEdgeProp = "androidboot.qemu.camera_hq_edge_processing";
        qemuDisplaySettingsXmlProp = "androidboot.qemu.display.settings.xml";
        qemuVirtioWifiProp = "androidboot.qemu.virtiowifi";
        qemuWifiProp = "androidboot.qemu.wifi";
        androidQemudProp = nullptr;  // deprecated
        qemuHwcodecAvcdecProp = "androidboot.qemu.hwcodec.avcdec";
        qemuHwcodecVpxdecProp = "androidboot.qemu.hwcodec.vpxdec";
        androidbootLogcatProp = "androidboot.logcat";
        adbKeyProp = "androidboot.qemu.adb.pubkey";
        avdNameProp = "androidboot.qemu.avd_name";
        deviceStateProp = "androidboot.qemu.device_state";
        qemuCpuVulkanVersionProp = "androidboot.qemu.cpuvulkan.version";
    } else {
        androidbootVerityMode = nullptr;
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
        qemuCameraHqEdgeProp = "qemu.camera_hq_edge_processing";
        qemuDisplaySettingsXmlProp = "qemu.display.settings.xml";
        qemuVirtioWifiProp = "qemu.virtiowifi";
        qemuWifiProp = "qemu.wifi";
        androidQemudProp = "android.qemud";
        qemuHwcodecAvcdecProp = "qemu.hwcodec.avcdec";
        qemuHwcodecVpxdecProp = "qemu.hwcodec.vpxdec";
        androidbootLogcatProp = nullptr;
        adbKeyProp = nullptr;
        avdNameProp = "qemu.avd_name";
        deviceStateProp = "qemu.device_state";
        qemuCpuVulkanVersionProp = nullptr;
    }

    std::vector<std::pair<std::string, std::string>> params;

    // We always force qemu=1 when running inside QEMU.
    if (fc::isEnabled(fc::AndroidbootProps2)) {
        params.push_back({"androidboot.qemu", "1"});
    } else {
        params.push_back({"qemu", "1"});
    }

    params.push_back({"androidboot.hardware", isQemu2 ? "ranchu" : "goldfish"});

    if (opts->guest_angle) {
        params.push_back({"androidboot.hardwareegl", "angle"});
    }

    if (serialno) {
        params.push_back({"androidboot.serialno", serialno});
    }

    if (opts->dalvik_vm_checkjni) {
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

    if (qemuCpuVulkanVersionProp
            && glesMode == kAndroidGlesEmulationHost) {
        // Put our swiftshader version string there, which is currently
        // Vulkan 1.1 (0x402000)
        params.push_back({
            qemuCpuVulkanVersionProp,StringFormat("%d", 0x402000)
            });
    }


    const char* pTimeout = avdInfo_screen_off_timeout(apiLevel);
    params.push_back({qemuScreenOffTimeoutProp, pTimeout});
    if (apiLevel >= 31 && androidbootVerityMode) {
        params.push_back({androidbootVerityMode, "enforcing"});
    }

    if (isQemu2 && fc::isEnabled(fc::EncryptUserData) && qemuEncryptProp) {
        params.push_back({qemuEncryptProp, "1"});
    }

    // Android media profile selection
    // 1. If the SelectMediaProfileConfig is on, then select
    // <media_profile_name> if the resolution is above 1080p (1920x1080).
    if (isQemu2 && fc::isEnabled(fc::DynamicMediaProfile) && qemuMediaProfileVideoProp) {
        if ((hw->hw_lcd_width > 1920 && hw->hw_lcd_height > 1080) ||
            (hw->hw_lcd_width > 1080 && hw->hw_lcd_height > 1920)) {
            dwarning("Display resolution > 1080p. Using different media profile.");
            params.push_back({
                qemuMediaProfileVideoProp,
                "/data/vendor/etc/media_codecs_google_video_v2.xml"
            });
        }
    }

    // Set vsync rate
    params.push_back({qemuVsyncProp, StringFormat("%u", hw->hw_lcd_vsync)});

    // Set gl transport props
    params.push_back({qemuGltransportNameProp, hw->hw_gltransport});
    params.push_back({
        qemuDrawFlushIntervalProp,
        StringFormat("%u", hw->hw_gltransport_drawFlushInterval)});

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

    // Send adb public key to device
    if (adbKeyProp) {
        auto privkey = getPrivateAdbKeyPath();
        std::string key = "";

        if (!privkey.empty() && pubkey_from_privkey(privkey, &key)) {
            params.push_back({adbKeyProp, key});
            LOG(INFO) << "Sending adb public key [" << key << "]";
        } else {
            LOG(WARNING) << "No adb private key exists";
        }
    }

    if (opts->bootchart) {
        params.push_back({"androidboot.bootchart", opts->bootchart});
    }

    if (opts->selinux) {
        params.push_back({"androidboot.selinux", opts->selinux});
    }

    if (hw->vm_heapSize > 0) {
        params.push_back({
            dalvikVmHeapsizeProp,
            StringFormat("%dm", hw->vm_heapSize)
        });
    }

    if (opts->legacy_fake_camera) {
        params.push_back({qemuLegacyFakeCameraProp, "1"});
    }

    if (apiLevel > 29) {
        params.push_back({qemuCameraProtocolVerProp, "1"});
    }

    if (!opts->camera_hq_edge) {
        params.push_back({qemuCameraHqEdgeProp, "0"});
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
    if (hw->display_settings_xml &&
        hw->display_settings_xml[0]
        && qemuDisplaySettingsXmlProp) {
        params.push_back({qemuDisplaySettingsXmlProp, hw->display_settings_xml});
    }

    if (resizableEnabled()) {
        params.push_back({qemuDisplaySettingsXmlProp, "resizable"});
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

    params.push_back({avdNameProp, hw->avd_name});

    if (deviceStateProp &&
        android::featurecontrol::isEnabled(android::featurecontrol::DeviceStateOnBoot)) {
        std::string deviceState = getDeviceStateString(hw);
        if (deviceState != "") {
            LOG(INFO) <<" sending device_state_config: " << deviceState;
            params.push_back({deviceStateProp, deviceState});
        }
    }

    for (auto i = opts->append_userspace_opt; i; i = i->next) {
        const char* const val = i->param;
        const char* const eq = strchr(val, '=');
        if (eq) {
            params.push_back({std::string(val, eq), eq + 1});
        } else {
            params.push_back({val, ""});
        }
    }
    return params;
}
