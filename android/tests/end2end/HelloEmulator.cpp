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
    void SetUp() override {
        System::get()->envSet("ANDROID_SDK_ROOT", "");
        System::get()->envSet("ANDROID_SDK_HOME", "");
        System::get()->envSet("ANDROID_HOME", "");
        mTempDir.reset(new TestTempDir("emuenvtest"));
    }

    void TearDown() override {
        mTempDir.reset();
        System::get()->envSet("ANDROID_SDK_ROOT", "");
        System::get()->envSet("ANDROID_SDK_HOME", "");
        System::get()->envSet("ANDROID_HOME", "");

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

        sdkSetup_copySkinFiles(root);
        sdkSetup_copySystemImageFiles(root);


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

        std::vector<std::string> allArgs = {
            emuLauncherPath,
            "-no-accel",
            "-no-snapshot",
            "-no-window",
            "-verbose",
            "-show-kernel",
        };

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

        (void)result;

        auto output = readFileIntoString(outFilePath);

        path_delete_file(outFilePath.c_str());

        if (output) {
            return *output;
        } else {
            return "";
        }
    }

    bool didConfigCheckSucceed(const std::string& output) {
        return output != "" &&
               (output.find("(does not exist)") == std::string::npos);
    }

    bool didEmulatorKernelStartup(const std::string& output) {
        // Look for markers of success---note, may be configuration
        // or implementation dependent
        bool hasMainLoopStartup =
            output.find("Starting QEMU main loop") != std::string::npos;

        bool hasColdBootChoice =
            output.find("Cold boot: requested by the user") != std::string::npos;

        return hasMainLoopStartup && hasColdBootChoice;
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
            /* hw.lcd.density */ 420,
            /* hw.lcd.height */ 1920,
            /* hw.lcd.width */ 1080,
            /* hw.mainKeys */ "no",
            /* hw.ramSize */ 1536,
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
            /* vm.heapSize */ 228,
        };

        generateAvd(genInfo);
    }

    std::string createAndLaunchAvd(StringView sdkRoot,
                                   StringView sdkHome,
                                   StringView androidTarget,
                                   StringView variant,
                                   StringView abi) {
        auto sdkRootPath = makeSdkAt(sdkRoot);
        auto sdkHomePath = makeSdkHomeAt(sdkHome);

        setSdkRoot(sdkRootPath);
        setSdkHome(sdkHomePath);

        const char avdName[] = "api19";

        generateAvdWithDefaults(
                avdName, sdkRootPath, sdkHomePath,
                "android-19", "google_apis", "armeabi-v7a");

        auto result =
            launchEmulatorWithResult(
                    {"-avd", avdName},
                    kLaunchTimeoutMs);

        // Print the result for posterity.
        printf("Kernel startup run result for avd %s:\n", avdName);
        printf("%s\n", result.c_str());

        return result;
    }

    std::unique_ptr<TestTempDir> mTempDir;
    std::vector<std::string> mCustomDirs;

private:
    std::string testdataSdkDir() {
        return pj(System::get()->getProgramDirectory(),
                  "testdata", "test-sdk");
    }

    // Copy the skin files over. Only nexus_5x supported for now.
    void sdkSetup_copySkinFiles(StringView sdkRoot) {
        auto skinDirSrc = pj(testdataSdkDir(), "skins", "nexus_5x");
        auto skinDirDst = pj(sdkRoot, "skins", "nexus_5x");

        path_mkdir_if_needed(skinDirDst.c_str(), 0755);

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
    }

    void sdkSetup_copySystemImageFiles(StringView sdkRoot) {
        // Only API 19 Google APIs ARMv7 supported for now.
        sdkSetup_copySystemImage(
            sdkRoot, "android-19", "google_apis", "armeabi-v7a");
    }

    void sdkSetup_copySystemImage(StringView sdkRoot,
                                  StringView androidTarget,
                                  StringView variant,
                                  StringView abi) {
        auto testSdkSysImgDir =
            pj(testdataSdkDir(),
               "system-images",
               androidTarget,
               variant,
               abi);

        auto testSdkSysImgDstDir =
            pj(sdkRoot,
               "system-images",
               androidTarget,
               variant,
               abi);

        path_mkdir_if_needed(testSdkSysImgDstDir.c_str(), 0755);

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
    }
};

TEST_F(EmulatorEnvironmentTest, BasicAccelCheck) {
    EXPECT_FALSE(
        launchEmulatorWithResult(
            {"-accel-check"},
            kLaunchTimeoutMs) == "");
}

TEST_F(EmulatorEnvironmentTest, BasicASCII) {
    auto sdkRootPath = makeSdkAt("testSdk");
    auto sdkHomePath = makeSdkHomeAt("testSdkHome");

    setSdkRoot(sdkRootPath);
    setSdkHome(sdkHomePath);

    auto result = launchEmulatorWithResult({"-config-check"}, kLaunchTimeoutMs);
    EXPECT_TRUE(didConfigCheckSucceed(result));
}

TEST_F(EmulatorEnvironmentTest, BasicNonASCII) {
    const char* sdkName = "\xF0\x9F\xA4\x94";
    const char* sdkHomeName = "foo\xE1\x80\x80 bar";

    auto sdkRootPath = makeSdkAt(sdkName);
    auto sdkHomePath = makeSdkHomeAt(sdkHomeName);

    setSdkRoot(sdkRootPath);
    setSdkHome(sdkHomePath);

    auto result = launchEmulatorWithResult({"-config-check"}, kLaunchTimeoutMs);
    EXPECT_TRUE(didConfigCheckSucceed(result));
}

TEST_F(EmulatorEnvironmentTest, BasicAvd) {

    std::string result =
        createAndLaunchAvd(
            "testSdk", "testSdkHome",
            "android-19", "google_apis",
            "armeabi-v7");

    EXPECT_TRUE(didEmulatorKernelStartup(result));
}

TEST_F(EmulatorEnvironmentTest, NonASCIIAvd) {
    const char* sdkName = "\xF0\x9F\xA4\x94";
    const char* sdkHomeName = "foo\xE1\x80\x80 bar";

    std::string result =
        createAndLaunchAvd(
            sdkName, sdkHomeName,
            "android-19", "google_apis",
            "armeabi-v7");

    EXPECT_TRUE(didEmulatorKernelStartup(result));
}

} // namespace base
} // namespace android
