// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/FileShareOpen.h"
#include "android/base/files/FileShareOpenImpl.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>
#include <memory>

namespace {

class FileShareTest : public ::testing::Test {
public:
    void SetUp() override {
        // Test non-ascii path
        mTempDir.reset(new android::base::TestTempDir("fileShareTest中文"));
        mFilePath = mTempDir->makeSubPath(kFileName);
        EXPECT_TRUE(mTempDir->makeSubFile(kFileName));
    }
    void TearDown() override { mTempDir.reset(); }

protected:
    std::unique_ptr<android::base::TestTempDir> mTempDir;
    std::string mFilePath;
    const char* kFileName = "test.txt";
};

}  // namespace

using android::base::FileShare;
using android::base::fsopen;

TEST_F(FileShareTest, shareRead) {
    FILE* f1 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f1) << "File: " << mFilePath << " was not opened: errno: " << errno;
    FILE* f2 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f2);
    if (f1) {
        fclose(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

TEST_F(FileShareTest, readWriteRefuse) {
    FILE* f1 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f1) << "File: " << mFilePath << " was not opened: errno: " << errno;
    FILE* f2 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_FALSE(f2) << "File: " << mFilePath << " was not opened: errno: " << errno;
    if (f1) {
        fclose(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

TEST_F(FileShareTest, writeWriteRefuse) {
    FILE* f1 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_TRUE(f1);
    FILE* f2 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_FALSE(f2);
    if (f1) {
        fclose(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

TEST_F(FileShareTest, writeReadRefuse) {
    FILE* f1 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_TRUE(f1) << "File: " << mFilePath << " was not opened: errno: " << errno;
    FILE* f2 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_FALSE(f2);
    if (f1) {
        fclose(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

TEST_F(FileShareTest, createShareRead) {
    void* f1 = android::base::internal::openFileForShare(mFilePath.c_str());
    EXPECT_TRUE(f1);
    FILE* f2 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f2);
    if (f1) {
        android::base::internal::closeFileForShare(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

TEST_F(FileShareTest, createShareWrite) {
    void* f1 = android::base::internal::openFileForShare(mFilePath.c_str());
    EXPECT_TRUE(f1);
    FILE* f2 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_TRUE(f2);
    if (f1) {
        android::base::internal::closeFileForShare(f1);
    }
    if (f2) {
        fclose(f2);
    }
}

#ifndef _WIN32
TEST_F(FileShareTest, updateShareReadToWrite) {
    FILE* f1 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f1);
    EXPECT_TRUE(updateFileShare(f1, FileShare::Write));
    FILE* f2 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_FALSE(f2);
    fclose(f1);
}

TEST_F(FileShareTest, updateShareWriteToRead) {
    FILE* f1 = fsopen(mFilePath.c_str(), "w", FileShare::Write);
    EXPECT_TRUE(f1);
    EXPECT_TRUE(updateFileShare(f1, FileShare::Read));
    FILE* f2 = fsopen(mFilePath.c_str(), "r", FileShare::Read);
    EXPECT_TRUE(f2);
    fclose(f1);
    fclose(f2);
}

#endif  // ifndef _WIN32
