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

#include "android/utils/filelock.h"

#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>
#include <unistd.h>

#ifndef _WIN32

namespace {
    class FileLockTest : public ::testing::Test {
    public:
        void SetUp() {
            const char* kFileLockName = "filelock.txt";
            mTempDir.reset(new android::base::TestTempDir("FileLockTest"));
            mFileLockPath = mTempDir->makeSubPath(kFileLockName);
            ASSERT_TRUE(mTempDir->makeSubFile(kFileLockName));
            ASSERT_EQ(pipe(mPipe0), 0);
            ASSERT_EQ(pipe(mPipe1), 0);
            mPid = fork();
            ASSERT_GE(mPid, 0);
            filelock_init();
        }
        void TearDown() {
            close(mPipe0[0]);
            close(mPipe0[1]);
            close(mPipe1[0]);
            close(mPipe1[1]);
            if (mPid > 0) {
                exit(0);
            }
        }
    protected:
        int mPid = 0;
        int mPipe0[2] = {};
        int mPipe1[2] = {};
        std::unique_ptr<android::base::TestTempDir> mTempDir;
        std::string mFileLockPath;
        bool isParentProcess() const {
            return mPid == 0;
        }
        size_t writeToOther(void* buffer, size_t size) {
            int pipe = isParentProcess() ? mPipe0[1] : mPipe1[1];
            return write(pipe, buffer, size);
        }
        size_t readFromOther(void* buffer, size_t size) {
            int pipe = isParentProcess() ? mPipe1[0] : mPipe0[0];
            return read(pipe, buffer, size);
        }
    };
}

TEST_F(FileLockTest, createConflict) {
    if (isParentProcess()) {
        // Parent process
        FileLock* lock = filelock_create(mFileLockPath.c_str());
        ASSERT_NE(lock, nullptr);
        uint8_t ready = 1;
        writeToOther(&ready, sizeof(uint8_t));
        uint8_t succeed = 0;
        readFromOther(&succeed, sizeof(uint8_t));
        ASSERT_EQ(succeed, 1);
        filelock_release(lock);
    } else {
        // Child process
        uint8_t ready;
        uint8_t succeed = 1;
        readFromOther(&ready, sizeof(uint8_t));
        EXPECT_EQ(ready, 1);
        succeed &= ready == 1;
        FileLock* lock = filelock_create(mFileLockPath.c_str());
        EXPECT_EQ(lock, nullptr);
        succeed &= lock == nullptr;
        if (lock) {
            filelock_release(lock);
        }
        // We must report the succeed bit back to parent process.
        // Or the test scheduler cannot catch it.
        writeToOther(&succeed, sizeof(uint8_t));
    }
}

TEST_F(FileLockTest, createAndRelease) {
    if (isParentProcess()) {
        // Parent process
        FileLock* lock = filelock_create(mFileLockPath.c_str());
        ASSERT_NE(lock, nullptr);
        filelock_release(lock);
        uint8_t ready = 1;
        writeToOther(&ready, sizeof(uint8_t));
        uint8_t succeed = 0;
        readFromOther(&succeed, sizeof(uint8_t));
        ASSERT_EQ(succeed, 1);
    } else {
        // Child process
        uint8_t ready;
        uint8_t succeed = 1;
        readFromOther(&ready, sizeof(uint8_t));
        EXPECT_EQ(ready, 1);
        succeed &= ready == 1;
        FileLock* lock = filelock_create(mFileLockPath.c_str());
        EXPECT_NE(lock, nullptr);
        succeed &= lock != nullptr;
        if (lock) {
            filelock_release(lock);
        }
        // We must report the succeed bit back to parent process.
        // Or the test scheduler cannot catch it.
        writeToOther(&succeed, sizeof(uint8_t));
    }
}

TEST_F(FileLockTest, waitForStale) {
    if (isParentProcess()) {
        // Parent process
        FileLock* lock = filelock_create(mFileLockPath.c_str());
        ASSERT_NE(lock, nullptr);
        uint8_t ready = 1;
        writeToOther(&ready, sizeof(uint8_t));
        android::base::System::get()->sleepMs(500);
        filelock_release(lock);
        uint8_t succeed = 0;
        readFromOther(&succeed, sizeof(uint8_t));
        ASSERT_EQ(succeed, 1);
    } else {
        // Child process
        uint8_t ready;
        uint8_t succeed = 1;
        readFromOther(&ready, sizeof(uint8_t));
        EXPECT_EQ(ready, 1);
        succeed &= ready == 1;
        FileLock* lock = filelock_create_timeout(mFileLockPath.c_str(), 1000);
        EXPECT_NE(lock, nullptr);
        succeed &= lock != nullptr;
        if (lock) {
            filelock_release(lock);
        }
        // We must report the succeed bit back to parent process.
        // Or the test scheduler cannot catch it.
        writeToOther(&succeed, sizeof(uint8_t));
    }
}

#endif

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
