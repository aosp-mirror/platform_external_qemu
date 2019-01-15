/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "android/avd/BugreportInfo.h"

#include "android/avd/info.h"
#include "android/avd/keys.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/files/IniFile.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/globals.h"
#include "android/metrics/StudioConfig.h"
#include "android/opengl/gpuinfo.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/system.h"

using android::base::IniFile;
using android::base::StringAppendFormat;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::trim;
using android::base::Version;
using android::update_check::VersionExtractor;

namespace android {
namespace avd {

BugreportInfo::BugreportInfo() {
    VersionExtractor vEx;
    Version curEmuVersion = vEx.getCurrentVersion();
    emulatorVer =
            curEmuVersion.isValid() ? curEmuVersion.toString() : "Unknown";
    android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();
    Version accelVersion = android::GetCurrentCpuAcceleratorVersion();
    if (accelVersion.isValid()) {
        hypervisorVer =
                (StringFormat("%s %s", android::CpuAcceleratorToString(accel),
                              accelVersion.toString()));
    }
    char versionString[128];
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    avdInfo_getFullApiName(apiLevel, versionString, 128);
    androidVer = versionString;

    Version studioVersion = android::studio::lastestAndroidStudioVersion();
    androidStduioVer =
            studioVersion.isValid() ? studioVersion.toString() : "Unknown";
    deviceName = StringView(avdInfo_getName(android_avdInfo));
    hostOsName = System::get()->getOsName();
    sdkToolsVer = android::getCurrentSdkVersion(
                          android::ConfigDirs::getSdkRootDirectory(),
                          android::SdkComponentType::Tools)
                          .toString();
    cpuModel = trim(android::GetCpuInfo().second);
    const auto buildProps = avdInfo_getBuildProperties(android_avdInfo);
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    buildFingerprint = ini.getString("ro.build.fingerprint", "");
    auto usage = System::get()->getMemUsage();
    totalMem = StringFormat("%d", (int)(usage.total_phys_memory / 1048576.0f));

    gpuListInfo = globalGpuInfoList().dump();

    static constexpr StringView kAvdDetailsNoDisplayKeys[] = {
            ABI_TYPE,    CPU_ARCH,    SKIN_NAME, SKIN_PATH,
            SDCARD_SIZE, SDCARD_PATH, IMAGES_2};

    if (const auto configIni = reinterpret_cast<const android::base::IniFile*>(
                avdInfo_getConfigIni(android_avdInfo))) {
        StringAppendFormat(&avdDetails, "Name: %s\n", deviceName);
        auto cpuArch = avdInfo_getTargetCpuArch(android_avdInfo);
        StringAppendFormat(&avdDetails, "CPU/ABI: %s\n", cpuArch);
        AFREE(cpuArch);
        StringAppendFormat(&avdDetails, "Path: %s\n",
                           avdInfo_getContentPath(android_avdInfo));
        auto tag = avdInfo_getTag(android_avdInfo);
        StringAppendFormat(&avdDetails, "Target: %s (API level %d)\n", tag,
                           avdInfo_getApiLevel(android_avdInfo));
        AFREE((char*)tag);

        char* skinName = nullptr;
        char* skinDir = nullptr;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (skinName) {
            StringAppendFormat(&avdDetails, "Skin: %s\n", skinName);
            AFREE(skinName);
        }
        AFREE(skinDir);

        const char* sdcard = avdInfo_getSdCardSize(android_avdInfo);
        if (!sdcard) {
            sdcard = avdInfo_getSdCardPath(android_avdInfo);
        }
        if (!sdcard || !*sdcard) {
            sdcard = android::base::strDup("<none>");
        }
        StringAppendFormat(&avdDetails, "SD Card: %s\n", sdcard);
        AFREE((char*)sdcard);

        if (configIni->hasKey(SNAPSHOT_PRESENT)) {
            StringAppendFormat(
                    &avdDetails, "Snapshot: %s",
                    avdInfo_getSnapshotPresent(android_avdInfo) ? "yes" : "no");
        }

        for (const std::string& key : *configIni) {
            const std::string& value = configIni->getString(key, "");
            // check if the key is already displayed
            if (std::find_if(std::begin(kAvdDetailsNoDisplayKeys),
                             std::end(kAvdDetailsNoDisplayKeys),
                             [&key](StringView noDisplayKey) {
                                 return noDisplayKey == key;
                             }) == std::end(kAvdDetailsNoDisplayKeys)) {
                StringAppendFormat(&avdDetails, "%s: %s\n", key, value);
            }
        }
    }
}

std::string BugreportInfo::dump() {
    return StringFormat(R"(Please Read:
https://developer.android.com/studio/report-bugs.html#emulator-bugs

Android Studio Version: %s

Emulator Version (Emulator--> Extended Controls--> Emulator Version): %s
HAXM / KVM Version: %s

Android SDK Tools: %s

Host Operating System: %s

CPU Manufacturer: %s

RAM: %s MB

GPU: %s

Build Fingerprint: %s

AVD Details: %s
)",
                        androidStduioVer, emulatorVer, hypervisorVer,
                        sdkToolsVer, hostOsName, cpuModel, totalMem,
                        gpuListInfo, buildFingerprint, avdDetails);
}
}  // namespace avd
}  // namespace android
