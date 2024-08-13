// Copyright (C) 2024 The Android Open Source Project
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

#include "utils/register_crash_info.h"
#include "android/avd/avd-info.h"
#include "android/base/files/IniFile.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/console.h"
#include "host-common/crash-handler.h"
#include "utils/register_crash_info.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

using android::base::IniFile;
using android::base::pj;
using android::base::TestTempDir;

namespace android {
namespace path {

TEST(RegisterCrashInfoTest, CollectCrashInfo) {
    // Create a temporary directory for the test.
    android::base::TestSystem testSystem("/bin", 64);
    testSystem.setCurrentDirectory("/home");
    TestTempDir* tempDir = testSystem.getTempRoot();

    // Create a test AvdInfo object.
    AvdInfo avd;

    tempDir->makeSubFile("config.ini");
    // Create a test config.ini file.
    std::string configIniPath = pj(tempDir->path(), "config.ini");
    IniFile configIni(configIniPath);
    configIni.setString("disk.dataPartition.size", "2048");
    configIni.setString("hw.cpu.ncore", "4");
    configIni.setString("hw.gpu.enabled", "yes");
    configIni.setString("hw.gpu.mode", "auto");
    configIni.setString("hw.lcd.height", "1920");
    configIni.setString("hw.lcd.width", "1080");
    configIni.setString("hw.ramSize", "4096");
    configIni.setString("hw.sensor.hinge", "hinge");
    configIni.setString("PlayStore.enabled", "true");
    ASSERT_TRUE(configIni.write());

    avd.configIni = reinterpret_cast<CIniFile*>(&configIni);
    avd.coreHardwareIniPath = configIniPath.data();

    // // Set the config.ini path for the AvdInfo object.
    // avdInfo_setConfigIni(avd, reinterpret_cast<void*>(&configIni));

    // Create a test build.prop file.
    std::string buildPropPath = pj(tempDir->path(), "build.prop");
    IniFile buildProp(buildPropPath);
    buildProp.setString(
            "ro.build.fingerprint",
            "Android/aosp_angler/angler:7.1.1/NYC/enh12211018:eng/test-keys");
    ASSERT_TRUE(buildProp.write());

    fileData_initFromFile(avd.buildProperties, buildPropPath.c_str());

    // Call collectCrashInfo and verify the results.
    auto info = collectCrashInfo(&avd);
    ASSERT_EQ(info["hw.cpu.ncore"], "4");
    ASSERT_EQ(info["hw.ramSize"], "4096");
    ASSERT_EQ(info["hw.lcd.width"], "1080");
    ASSERT_EQ(info["hw.lcd.height"], "1920");
    ASSERT_EQ(info["PlayStore.enabled"], "true");
    ASSERT_EQ(info["disk.dataPartition.size"], "2048");
    ASSERT_EQ(info["ro.build.fingerprint"],
              "Android/aosp_angler/angler:7.1.1/NYC/enh12211018:eng/test-keys");
    ASSERT_NE(info["cpu_cores"], "??");
    ASSERT_NE(info["free_ram"], "??");
    ASSERT_NE(info["avd_space"], "??");
}

}  // namespace path
}  // namespace android
