// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/tempfile.h"

#include "android/base/StringView.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

static bool fileExists(android::base::StringView filename) {
    return android::base::System::get()->pathExists(filename);
}

TEST(tempfile, createTemp) {
    TempFile* tempFile = tempfile_create();
    const char* filePath = tempfile_path(tempFile);
    EXPECT_TRUE(fileExists(filePath));
    tempfile_close(tempFile);
    EXPECT_FALSE(fileExists(filePath));
}

TEST(tempfile, createTempWithExt) {
    const char* ext = ".ext";
    TempFile* tempFile = tempfile_create_with_ext(ext);
    const char* filePath = tempfile_path(tempFile);
    EXPECT_TRUE(fileExists(filePath));
    int pos = strlen(filePath) - strlen(ext);
    EXPECT_GT(pos, 0);
    EXPECT_EQ(0, strcmp(filePath + pos, ext));
    tempfile_close(tempFile);
    EXPECT_FALSE(fileExists(filePath));
}

TEST(tempfile, createAndDoubleClose) {
    TempFile* tempFile = tempfile_create();
    std::string filePath = tempfile_path(tempFile);
    EXPECT_TRUE(fileExists(filePath));
    tempfile_unref_and_close(filePath.c_str());
    EXPECT_FALSE(fileExists(filePath));
    // Expect no error or anything
    tempfile_unref_and_close(filePath.c_str());
    EXPECT_FALSE(fileExists(filePath));
}

TEST(tempfile, badClose) {
    std::string gtestTmpDir = android::base::System::get()->getTempDir();
    const char* filename = "tempfile";
    const std::string filePath =
            android::base::PathUtils::join(gtestTmpDir, filename);
    FILE* fd = android_fopen(filePath.c_str(), "w");
    fclose(fd);
    EXPECT_TRUE(fileExists(filePath));
    tempfile_unref_and_close(filePath.c_str());
    EXPECT_TRUE(fileExists(filePath));
    EXPECT_TRUE(android::base::System::get()->deleteFile(filePath));
    EXPECT_FALSE(fileExists(filePath));
}
