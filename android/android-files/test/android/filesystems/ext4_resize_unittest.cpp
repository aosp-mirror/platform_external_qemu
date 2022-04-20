// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/filesystems/ext4_resize.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/testing/Utils.h"
#include "android/filesystems/ext4_utils.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using android::base::StringView;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;
using std::string;
using std::unique_ptr;

// Initial image size needs to be > 200MB
static const int64_t kInitialImageSize = 210 * 1024 * 1024;

// Workaround to make sure that we are looking for e2fsck in the
// subdirectory of where the unittest is located.
//
class ResolvingTestSystem : public TestSystem {
public:
    ResolvingTestSystem()
        // We explicitly set the launcher dir, as we expect the resize
        // executable to be in a directory below us.
        : TestSystem(System::get()->getProgramDirectory(),
                     System::kProgramBitness) {}

    // Make sure we use real path resolution vs, test system one.
    virtual bool pathIsFile(StringView path) const override {
       return hostSystem()->pathIsFile(path);
    }

    // Run commands using original system, vs. test system
    bool runCommand(const std::vector<std::string>& commandLine,
                    RunOptions options,
                    System::Duration timeoutMs,
                    System::ProcessExitCode* outExitCode,
                    System::Pid* outChildPid,
                    const std::string& outputFile) override {
        return hostSystem()->runCommand(commandLine, options, timeoutMs, outExitCode, outChildPid, outputFile);
    }
};

class Ext4ResizeTest : public ::testing::Test {
public:
    Ext4ResizeTest() : test() {}

    void SetUp() override {
        SKIP_TEST_ON_WINE();

        static const char kSubPath[] = "testImage.img";
        // Code coverage can interfere with detection of the resize exectuable
        mTempDir.reset(new TestTempDir("ext4resizetest"));
        ASSERT_TRUE(mTempDir->path());
        mFilePath = mTempDir->makeSubPath(kSubPath);
        ASSERT_EQ(0,
                  android_createEmptyExt4Image(mFilePath.c_str(),
                                               kInitialImageSize, "oogabooga"));
        // Precondition
        expectValidImage(kInitialImageSize);
    }

    void expectValidImage(System::FileSize expectedSize) {
        EXPECT_TRUE(android_pathIsExt4PartitionImage(mFilePath.c_str()));
        System::FileSize size;
        EXPECT_TRUE(System::get()->pathFileSize(mFilePath, &size));
        EXPECT_EQ(expectedSize, size);
    }

protected:
    unique_ptr<TestTempDir> mTempDir;
    string mFilePath;
    ResolvingTestSystem test;
};

// This test crashes deep in a wine's kernel stack.
TEST_F(Ext4ResizeTest, enlarge) {
    SKIP_TEST_ON_WINE();
    static const int64_t kLargerSize = 1024 * 1024 * 1024;
    EXPECT_EQ(0, resizeExt4Partition(mFilePath.c_str(), kLargerSize));
    expectValidImage(kLargerSize);
}

// This test crashes deep in a wine's kernel stack.
TEST_F(Ext4ResizeTest, contract) {
    SKIP_TEST_ON_WINE();
    static const int64_t kSmallerSize = 201 * 1024 * 1024;
    EXPECT_EQ(0, resizeExt4Partition(mFilePath.c_str(), kSmallerSize));
    expectValidImage(kSmallerSize);
}
