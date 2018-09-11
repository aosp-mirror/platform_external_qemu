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

#include <stdio.h>

using android::base::IniFile;
using android::base::pj;

void generateAvd(const AvdGenerateInfo& genInfo) {
    std::string name = genInfo.name;
    std::string avdRoot =
        pj(genInfo.sdkHomeDir, "avd");
    std::string relPath =
        pj("avd", name + ".avd");

    std::string avdIniPath = pj(avdRoot, name + ".ini");
    std::string avdDir = pj(avdRoot, name + ".avd");

    std::string configIniPath = pj(avdDir, "config.ini");

    if (path_mkdir_if_needed(avdRoot.c_str(), 0755) < 0) {
        fprintf(stderr, "%s: could not create dir %s!\n",
                __func__, avdDir.c_str());
        return;
    }

    if (path_mkdir_if_needed(avdDir.c_str(), 0755) < 0) {
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
