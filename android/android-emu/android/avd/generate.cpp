// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/avd/generate.h"

#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/utils/path.h"

#include <string>

#include <stdio.h>

using android::base::IniFile;
using android::base::pj;
using android::base::StringView;

void generateAvd(const AvdGenerateInfo& genInfo) {
    std::string name = genInfo.name;
    std::string avdRoot =
        pj(genInfo.sdkHomeDir, "avd");
    std::string relPath =
        pj("avd", name + ".avd");

    std::string avdIniPath = pj(avdRoot, name + ".ini");
    std::string avdDir = pj(avdRoot, name + ".avd");

    std::string configIniPath = pj(avdDir, "config.ini");

    if (path_mkdir_if_needed_no_cow(avdRoot.c_str(), 0755) < 0) {
        fprintf(stderr, "%s: could not create dir %s!\n",
                __func__, avdDir.c_str());
        return;
    }

    if (path_mkdir_if_needed_no_cow(avdDir.c_str(), 0755) < 0) {
        fprintf(stderr, "%s: could not create dir %s!\n",
                __func__, avdDir.c_str());
        return;
    }

    IniFile avdIni(avdIniPath);

    avdIni.setString("avd.ini.encoding", "UTF-8");
    avdIni.setString("path", avdDir);
    avdIni.setString("path.rel", relPath);
    avdIni.setString("target", genInfo.sysimgTarget);

    avdIni.write();

    IniFile configIni(configIniPath);

    configIni.setString("AvdId", name);
    configIni.setString("PlayStore.enabled", "false");
    configIni.setString("abi.type", genInfo.abi);
    configIni.setString("avd.ini.displayname", name);
    configIni.setString("avd.ini.encoding", genInfo.encoding);
    configIni.setString("disk.dataPartition.size", genInfo.dataPartitionSize);
    configIni.setString("fastboot.chosenSnapshotFile", genInfo.fastbootChosenSnapshotFile);
    configIni.setString("fastboot.forceChosenSnapshotBoot", genInfo.fastbootForceChosenSnapshotBoot);
    configIni.setString("fastboot.forceColdBoot", genInfo.fastbootForceColdBoot);
    configIni.setString("fastboot.forceFastBoot", genInfo.fastbootForceFastBoot);
    configIni.setString("hw.accelerometer", genInfo.hwAccelerometer);
    configIni.setString("hw.arc", genInfo.hwArc);
    configIni.setString("hw.audioInput", genInfo.hwAudioInput);
    configIni.setString("hw.audioOutput", genInfo.hwAudioOutput);
    configIni.setString("hw.battery", genInfo.hwBattery);
    configIni.setString("hw.camera.back", genInfo.hwCameraBack);
    configIni.setString("hw.camera.front", genInfo.hwCameraFront);
    configIni.setString("hw.cpu.arch", genInfo.hwCpuArch);
    configIni.setString("hw.cpu.model", genInfo.hwCpuModel);
    configIni.setInt("hw.cpu.ncore", genInfo.hwCpuNcore);
    configIni.setString("hw.dPad", genInfo.hwDpad);
    configIni.setString("hw.device.hash2", genInfo.hwDeviceHash2);
    configIni.setString("hw.device.manufacturer", genInfo.hwDeviceManufacturer);
    configIni.setString("hw.device.name", genInfo.hwDeviceName);
    configIni.setString("hw.gps", genInfo.hwGps);
    configIni.setString("hw.gpu.enabled", genInfo.hwGpuEnabled);
    configIni.setString("hw.gpu.mode", genInfo.hwGpuMode);
    configIni.setString("hw.initialOrientation", genInfo.hwInitialOrientation);
    configIni.setString("hw.keyboard", genInfo.hwKeyboard);
    configIni.setInt("hw.lcd.density", genInfo.hwLcdDensity);
    configIni.setInt("hw.lcd.height", genInfo.hwLcdHeight);
    configIni.setInt("hw.lcd.width", genInfo.hwLcdWidth);
    configIni.setString("hw.mainKeys", genInfo.hwMainKeys);
    configIni.setInt("hw.ramSize", genInfo.hwRamSize);
    configIni.setString("hw.sdCard", genInfo.hwSdCard);
    configIni.setString("hw.sensors.orientation", genInfo.hwSensorsOrientation);
    configIni.setString("hw.sensors.proximity", genInfo.hwSensorsProximity);
    configIni.setString("hw.trackBall", genInfo.hwTrackball);
    configIni.setString("image.sysdir.1", genInfo.imageSysdir1);
    configIni.setString("runtime.network.latency", genInfo.runtimeNetworkLatency);
    configIni.setString("runtime.network.speed", genInfo.runtimeNetworkSpeed);
    configIni.setString("sdcard.size", genInfo.sdcardSize);
    configIni.setString("showDeviceFrame", genInfo.showDeviceFrame);
    configIni.setString("skin.dynamic", genInfo.skinDynamic);
    configIni.setString("skin.name", genInfo.skinName);
    configIni.setString("skin.path", genInfo.skinPath);
    configIni.setString("tag.display", genInfo.tagDisplay);
    configIni.setString("tag.id", genInfo.tagId);
    configIni.setInt("vm.heapSize", genInfo.vmHeapSize);

    configIni.write();
}

void generateAvdWithDefaults(StringView avdName,
                             StringView sdkRootPath,
                             StringView sdkHomePath,
                             StringView androidTarget,
                             StringView variant,
                             StringView abi) {

    std::string cpuArch("arm");
    std::string cpuModel("cortex-a8");

    // TODO: Figure out the other correspondences
    // between system image abi, base arch, and cpu model
    if (abi.str() == "armeabi-v7a") {
        cpuArch = "arm";
        cpuModel = "cortex-a8";
    } else if (abi.str() == "arm64-v8a") {
        cpuArch = "arm";
        cpuModel = "cortex-a15";
    } else if (abi.str() == "x86") {
        cpuArch = "x86";
        cpuModel = "";
    } else if (abi.str() == "x86_64") {
        cpuArch = "x86_64";
        cpuModel = "";
    }

    std::string sysDir =
        pj(sdkRootPath,
           "system-images",
           androidTarget,
           variant,
           abi);

    std::string skinDir =
        pj(sdkRootPath,
           "skins",
           "nexus_5x");

    AvdGenerateInfo genInfo = {
        // sdk home dir, target
        c_str(sdkHomePath),
        c_str(androidTarget),

        // config.ini
        /* AvdId */ c_str(avdName),
        /* PlayStore.enabled */ "false",
        /* abi.type */ c_str(abi),
        /* avd.ini.displayname */ c_str(avdName),
        /* avd.ini.encoding */ "UTF-8",
        /* disk.dataPartition.size */ "1M",
        /* fastboot.chosenSnapshotFile */ "",
        /* fastboot.forceChosenSnapshotBoot */ "no",
        /* fastboot.forceColdBoot */ "no",
        /* fastboot.forceFastBoot */ "yes",
        /* hw.accelerometer */ "yes",
        /* hw.arc */ "false",
        /* hw.audioInput */ "yes",
        /* hw.audioOutput */ "yes",
        /* hw.battery */ "yes",
        /* hw.camera.back */ "virtualscene",
        /* hw.camera.front */ "emulated",
        /* hw.cpu.arch */ cpuArch.c_str(),
        /* hw.cpu.model */ cpuModel.c_str(),
        /* hw.cpu.ncore */ 4,
        /* hw.dPad */ "no",
        /* hw.device.hash2 */ "MD5:97c152442f4d6d2d0f1ac1110e207ea7",
        /* hw.device.manufacturer */ "Google",
        /* hw.device.name */ "Nexus 5X",
        /* hw.gps */ "yes",
        /* hw.gpu.enabled */ "yes",
        /* hw.gpu.mode */ "auto",
        /* hw.initialOrientation */ "Portrait",
        /* hw.keyboard */ "yes",
        /* hw.lcd.density */ 120,
        /* hw.lcd.height */ 480,
        /* hw.lcd.width */ 640,
        /* hw.mainKeys */ "no",
        /* hw.ramSize */ 128,
        /* hw.sdCard */ "yes",
        /* hw.sensors.orientation */ "yes",
        /* hw.sensors.proximity */ "yes",
        /* hw.trackBall */ "no",
        /* image.sysdir.1 */ sysDir.c_str(),
        /* runtime.network.latency */ "none",
        /* runtime.network.speed */ "full",
        /* sdcard.size */ "1M",
        /* showDeviceFrame */ "yes",
        /* skin.dynamic */ "yes",
        /* skin.name */ "nexus_5x",
        /* skin.path */ skinDir.c_str(),
        /* tag.display */ "Google APIs",
        /* tag.id */ "google_apis",
        /* vm.heapSize */ 16,
    };

    generateAvd(genInfo);
}

void deleteAvd(StringView avdName,
               StringView sdkHomePath) {
    auto avdIniPath =
        pj(sdkHomePath, "avd", avdName.str() + ".ini");
    auto avdFolderPath =
        pj(sdkHomePath, "avd", avdName.str() + ".avd");

    path_delete_file(avdIniPath.c_str());
    path_delete_dir(avdFolderPath.c_str());
}
