// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android-qemu2-glue/base/files/KernelCmdLineLoader.h"

#include <gtest/gtest.h>
#include <fstream>
#include <memory>
#include "aemu/base/files/PathUtils.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

using android::qemu::loadKernelCmdLine;

class LoadKernelCmdLineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary files for testing
        // TestSystem testSystem;
        // TestTempDir* testDir = testSystem.getTempRoot();

        std::ofstream emptyFile("empty_file.txt");
        emptyFile.close();

        std::ofstream contentFile("content_file.txt");
        contentFile << "some content with trailing spaces    \n";
        contentFile.close();

        std::ofstream unicodeFile(
                android::base::PathUtils::asUnicodePath("ünicode_file.txt")
                        .c_str());
        unicodeFile << "content in unicode file";
        unicodeFile.close();
    }

    void TearDown() override {
        // Clean up temporary files
        std::remove("empty_file.txt");
        std::remove("content_file.txt");
        std::remove("ünicode_file.txt");
    }
};

TEST_F(LoadKernelCmdLineTest, NullPath) {
    EXPECT_EQ(loadKernelCmdLine(nullptr), "");
}

TEST_F(LoadKernelCmdLineTest, NonExistentFile) {
    char* nonExistentPath = (char*)"non_existent_file.txt";
    EXPECT_EQ(loadKernelCmdLine(nonExistentPath), "");
}

TEST_F(LoadKernelCmdLineTest, EmptyFile) {
    char* emptyFilePath = (char*)"empty_file.txt";
    EXPECT_EQ(loadKernelCmdLine(emptyFilePath), "");
}

TEST_F(LoadKernelCmdLineTest, FileWithContent) {
    char* contentFilePath = (char*)"content_file.txt";
    EXPECT_EQ(loadKernelCmdLine(contentFilePath),
              "some content with trailing spaces");
}

TEST_F(LoadKernelCmdLineTest, UnicodePath) {
    char* unicodeFilePath = (char*)"ünicode_file.txt";
    EXPECT_EQ(loadKernelCmdLine(unicodeFilePath), "content in unicode file");
}
