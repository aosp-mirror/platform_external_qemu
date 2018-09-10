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
#include "android/base/files/PathUtils.h"
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

static const int kLaunchTimeoutMs = 5000;

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

        std::string emulatorBinaryFilename(EMU_BINARY_BASENAME EMU_BINARY_SUFFIX);
        auto emuLauncherPath = PathUtils::join(dir, emulatorBinaryFilename);
        EXPECT_TRUE(path_exists(emuLauncherPath.c_str()));

        std::vector<std::string> allArgs = { emuLauncherPath };

        for (const auto a: args) {
            allArgs.push_back(a);
        }

        std::string result =
            System::get()->runCommandWithResult(
                allArgs,
                timeoutMs).valueOr({});

        return result;
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
    fprintf(stderr, "%s: %s\n", __func__, result.c_str());
}

} // namespace base
} // namespace android
