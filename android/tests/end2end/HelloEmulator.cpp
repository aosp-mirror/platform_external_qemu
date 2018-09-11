// Copyright 2018 The Android Open Source Project
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
#include "android/base/files/PathUtils.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/path.h"

#include <memory>

#include <gtest/gtest.h>

#define EMU_BINARY_BASENAME "emulator"

#ifdef _WIN32
#define EMU_BINARY_SUFFIX ".exe"
#else
#define EMU_BINARY_SUFFIX ""
#endif

static const int kLaunchTimeoutMs = 10000;

namespace android {
namespace base {

class EmulatorEnvironmentTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("emuenvtest")); }

    void TearDown() override {
        mTempDir.reset();
        System::get()->envSet("ANDROID_SDK_ROOT", "");
        System::get()->envSet("ANDROID_SDK_HOME", "");

        for (const auto& dir: mCustomDirs) {
            path_delete_dir(dir.c_str());
        }
    }

    std::string makeSdkAt(StringView dir) {
        std::string root = mTempDir->makeSubPath(dir);
        std::string platforms = PathUtils::join(root, "platforms");
        std::string platformTools = PathUtils::join(root, "platform-tools");

        EXPECT_TRUE(path_mkdir_if_needed(platforms.c_str(), 0755) == 0);
        EXPECT_TRUE(path_mkdir_if_needed(platformTools.c_str(), 0755) == 0);

        mCustomDirs.push_back(root);

        return root;
    }

    std::string makeSdkHomeAt(StringView dir) {
        std::string root = mTempDir->makeSubPath(dir);
        std::string avdRoot = PathUtils::join(root, "avd");

        EXPECT_TRUE(path_mkdir_if_needed(root.c_str(), 0755) == 0);
        EXPECT_TRUE(path_mkdir_if_needed(avdRoot.c_str(), 0755) == 0);

        mCustomDirs.push_back(root);

        return root;
    }

    void setSdkRoot(StringView sdkRoot) {
        System::get()->envSet("ANDROID_SDK_ROOT", sdkRoot);
    }

    void setSdkHome(StringView sdkHome) {
        System::get()->envSet("ANDROID_SDK_HOME", sdkHome);
    }

    std::string launchEmulatorWithResult(
        const std::vector<std::string>& args,
        int timeoutMs) {

        auto dir = System::get()->getLauncherDirectory();

        auto outFilePath = pj(dir, "emuOutput.txt");

        std::string emulatorBinaryFilename(EMU_BINARY_BASENAME EMU_BINARY_SUFFIX);
        auto emuLauncherPath = PathUtils::join(dir, emulatorBinaryFilename);
        EXPECT_TRUE(path_exists(emuLauncherPath.c_str()));

        std::vector<std::string> allArgs = { emuLauncherPath };

        for (const auto a: args) {
            allArgs.push_back(a);
        }

        bool result =
            System::get()->runCommand(
                allArgs,
                RunOptions::WaitForCompletion |
                RunOptions::TerminateOnTimeout |
                RunOptions::DumpOutputToFile,
                timeoutMs,
                nullptr,
                nullptr,
                outFilePath);

        auto output = readFileIntoString(outFilePath);

        path_delete_file(outFilePath.c_str());

        if (output) {
            return *output;
        } else {
            return "";
        }
    }

    std::unique_ptr<TestTempDir> mTempDir;

    std::vector<std::string> mCustomDirs;
};

TEST_F(EmulatorEnvironmentTest, BasicAccelCheck) {
    EXPECT_FALSE(launchEmulatorWithResult({"-accel-check"}, kLaunchTimeoutMs) == "");
}

TEST_F(EmulatorEnvironmentTest, BasicASCII) {
    auto sdkRootPath = makeSdkAt("testSdk");
    auto sdkHomePath = makeSdkHomeAt("testSdkHome");

    setSdkRoot(sdkRootPath);
    setSdkHome(sdkHomePath);

    auto result = launchEmulatorWithResult({"-config-check"}, kLaunchTimeoutMs);
    EXPECT_FALSE(result == "");
}

TEST_F(EmulatorEnvironmentTest, BasicNonASCII) {
    const char* sdkName = "\xF0\x9F\xA4\x94";
    const char* sdkHomeName = "foo\xE1\x80\x80 bar";

    auto sdkRootPath = makeSdkAt(sdkName);
    auto sdkHomePath = makeSdkHomeAt(sdkHomeName);

    setSdkRoot(sdkRootPath);
    setSdkHome(sdkHomePath);

    fprintf(stderr, "%s: %s\n", __func__, sdkRootPath.c_str());

    // MessageBox(0, "Hello there", "want to continue?", MB_OK);

    auto result = launchEmulatorWithResult({"-config-check"}, kLaunchTimeoutMs);
    EXPECT_FALSE(result == "");
    fprintf(stderr, "%s: res: %s\n", __func__, result.c_str());
}

static std::string testdataSdkDir() {
    return pj(System::get()->getProgramDirectory(), "testdata", "test-sdk");
}

TEST_F(EmulatorEnvironmentTest, GenerateAvd) {
    auto sdkRootPath = makeSdkAt("testdataSdkDir");
    auto sdkHomePath = makeSdkHomeAt("testSdkHome");

    auto skinDirSrc = pj(testdataSdkDir(), "skins", "nexus_5x");
    auto skinDirDst = pj(sdkRootPath, "skins", "nexus_5x");

    auto testSdkSysImgDir =
        pj(testdataSdkDir(),
           "system-images",
           "android-19",
           "google_apis",
           "armeabi-v7a");

    auto testSdkSysImgDstDir =
        pj(sdkRootPath,
           "system-images",
           "android-19",
           "google_apis",
           "armeabi-v7a");

    path_mkdir_if_needed(skinDirDst.c_str(), 0755);
    path_mkdir_if_needed(testSdkSysImgDstDir.c_str(), 0755);

    std::vector<std::string> skinFiles = {
        "land_back.webp",
        "land_fore.webp",
        "land_shadow.webp",
        "layout",
        "port_back.webp",
        "port_fore.webp",
        "port_shadow.webp",
    };

    for (const auto f: skinFiles) {
        auto src = pj(skinDirSrc, f);
        auto dst = pj(skinDirDst, f);
        path_copy_file(dst.c_str(), src.c_str());
    }

    std::vector<std::string> sysimgFiles = {
        "NOTICE.txt",
        "build.prop",
        "kernel-ranchu",
        "package.xml",
        "ramdisk.img",
        "source.properties",
        "system.img",
        "userdata.img",
    };

    for (const auto f: sysimgFiles) {
        auto src = pj(testSdkSysImgDir, f);
        auto dst = pj(testSdkSysImgDstDir, f);
        path_copy_file(dst.c_str(), src.c_str());
    }

    setSdkRoot(sdkRootPath);
    setSdkHome(sdkHomePath);

    auto skinPath = pj(sdkRootPath, "skins", "nexus_5x");

    AvdGenerateInfo genInfo = {
        // sdk home dir, target
        sdkHomePath.c_str(),
        // sys img target
        "android-19",

        // config.ini
        /* AvdId */ "api19arm",
        /* PlayStore.enabled */ "false",
        /* abi.type */ "armeabi-v7a",
        /* avd.ini.displayname */ "api19arm",
        /* avd.ini.encoding */ "UTF-8",
        /* disk.dataPartition.size */ "2G",
        /* fastboot.chosenSnapshotFile */ "",
        /* fastboot.forceChosenSnapshotBoot */ "no",
        /* fastboot.forceColdBoot */ "no",
        /* fastboot.forceFastBoot */ "yes",
        /* hw.accelerometer */ "yes",
        /* hw.arc */ "false",
        /* hw.audioInput */ "yes",
        /* hw.battery */ "yes",
        /* hw.camera.back */ "virtualscene",
        /* hw.camera.front */ "emulated",
        /* hw.cpu.arch */ "arm",
        /* hw.cpu.model */ "cortex-a8",
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
        /* hw.lcd.density */ 420,
        /* hw.lcd.height */ 1920,
        /* hw.lcd.width */ 1080,
        /* hw.mainKeys */ "no",
        /* hw.ramSize */ 1536,
        /* hw.sdCard */ "yes",
        /* hw.sensors.orientation */ "yes",
        /* hw.sensors.proximity */ "yes",
        /* hw.trackBall */ "no",
        /* image.sysdir.1 */ "system-images/android-19/google_apis/armeabi-v7a/",
        /* runtime.network.latency */ "none",
        /* runtime.network.speed */ "full",
        /* sdcard.size */ "512M",
        /* showDeviceFrame */ "yes",
        /* skin.dynamic */ "yes",
        /* skin.name */ "nexus_5x",
        /* skin.path */ skinPath.c_str(),
        /* tag.display */ "Google APIs",
        /* tag.id */ "google_apis",
        /* vm.heapSize */ 228,
    };

    generateAvd(genInfo);

    auto ainipath = pj(sdkHomePath, "avd", "api19arm.ini");

    auto aIni = readFileIntoString(ainipath);
    if (aIni) {
        fprintf(stderr, "%s: aini %s\n", __func__, aIni->c_str());
    } else {
        fprintf(stderr, "%s: aini notfound\n", __func__);
    }

    auto inipath = pj(sdkHomePath, "avd", "api19arm.avd", "config.ini");

    fprintf(stderr, "%s: ini path: %s\n", __func__, inipath.c_str());

    auto configIni = readFileIntoString(pj(sdkHomePath, "avd", "api19arm.avd", "config.ini"));

    if (configIni) {
        fprintf(stderr, "%s: cfg %s\n", __func__, configIni->c_str());
    } else {
        fprintf(stderr, "%s: cfg notfound\n", __func__);
    }

    auto result = launchEmulatorWithResult({"-verbose", "-avd", "api19arm", "-show-kernel", "-no-snapshot", "-no-window"}, kLaunchTimeoutMs);
    EXPECT_FALSE(result == "");
    fprintf(stderr, "%s: res: %s\n", __func__, result.c_str());
}

} // namespace base
} // namespace android
