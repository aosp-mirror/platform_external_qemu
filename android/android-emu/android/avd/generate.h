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
#pragma once

#include "android/base/StringView.h"

// Includes enough for <avdname>.ini and config.ini
// Examples shown alongside.
// Only purely integer fields are not strings.
struct AvdGenerateInfo {
    const char* sdkHomeDir;
    // Target system image (android-19, android-26, etc.)
    const char* sysimgTarget;
    // config.ini
    const char* name; // AvdId=api26
    const char* playStoreEnabled; // PlayStore.enabled=false
    const char* abi; // abi.type=x86
    const char* displayname; // avd.ini.displayname=api26
    const char* encoding; // avd.ini.encoding=UTF-8
    const char* dataPartitionSize; // disk.dataPartition.size=2G
    const char* fastbootChosenSnapshotFile; // fastboot.chosenSnapshotFile=
    const char* fastbootForceChosenSnapshotBoot; // fastboot.forceChosenSnapshotBoot=no
    const char* fastbootForceColdBoot; // fastboot.forceColdBoot=no
    const char* fastbootForceFastBoot; // fastboot.forceFastBoot=yes
    const char* hwAccelerometer; // hw.accelerometer=yes
    const char* hwArc; // hw.arc=false
    const char* hwAudioInput; // hw.audioInput=yes
    const char* hwAudioOutput; // hw.audioOutput=yes
    const char* hwBattery; // hw.battery=yes
    const char* hwCameraBack; // hw.camera.back=virtualscene
    const char* hwCameraFront; // hw.camera.front=emulated
    const char* hwCpuArch; // hw.cpu.arch=x86
    const char* hwCpuModel; // hw.cpu.model=???
    int hwCpuNcore; // hw.cpu.ncore=4
    const char* hwDpad; // hw.dPad=no
    const char* hwDeviceHash2; // hw.device.hash2=MD5:bc5032b2a871da511332401af3ac6bb0
    const char* hwDeviceManufacturer; // hw.device.manufacturer=Google
    const char* hwDeviceName; // hw.device.name=Nexus 5X
    const char* hwGps; // hw.gps=yes
    const char* hwGpuEnabled; // hw.gpu.enabled=yes
    const char* hwGpuMode; // hw.gpu.mode=auto
    const char* hwInitialOrientation; // hw.initialOrientation=Portrait
    const char* hwKeyboard; // hw.keyboard=yes
    int hwLcdDensity; // hw.lcd.density=420
    int hwLcdHeight; // hw.lcd.height=1920
    int hwLcdWidth; // hw.lcd.width=1080
    const char* hwMainKeys; // hw.mainKeys=no
    int hwRamSize; // hw.ramSize=1536
    const char* hwSdCard; // hw.sdCard=yes
    const char* hwSensorsOrientation; // hw.sensors.orientation=yes
    const char* hwSensorsProximity; // hw.sensors.proximity=yes
    const char* hwTrackball; // hw.trackBall=no
    const char* imageSysdir1; // image.sysdir.1=system-images\android-26\google_apis_playstore\x86
    const char* runtimeNetworkLatency; // runtime.network.latency=none
    const char* runtimeNetworkSpeed; // runtime.network.speed=full
    const char* sdcardSize; // sdcard.size=512M
    const char* showDeviceFrame; // showDeviceFrame=yes
    const char* skinDynamic; // skin.dynamic=yes
    const char* skinName; // skin.name=nexus_5x
    const char* skinPath; // skin.path=A:\android-sdk\skins\nexus_5x
    const char* tagDisplay; // tag.display=Google Play
    const char* tagId; // tag.id=google_apis_playstore
    int vmHeapSize; // vm.heapSize=228
};

void generateAvd(const AvdGenerateInfo& avdGenerateInfo);

void generateAvdWithDefaults(android::base::StringView avdName,
                             android::base::StringView sdkRootPath,
                             android::base::StringView sdkHomePath,
                             android::base::StringView androidTarget,
                             android::base::StringView variant,
                             android::base::StringView abi);

void deleteAvd(android::base::StringView avdName,
               android::base::StringView sdkHomePath);
