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
#include "android/utils/path.h"

#include <gtest/gtest.h>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define TEST_F_WITH_NAME(test_fixture, test_name)               \
    GTEST_TEST_WITH_NAME(test_fixture, test_name, test_fixture, \
                         ::testing::internal::GetTypeId<test_fixture>())

#define GTEST_TEST_WITH_NAME(test_case_name, test_name, parent_class,          \
                             parent_id)                                        \
    class GTEST_TEST_CLASS_NAME_(test_case_name, test_name)                    \
        : public parent_class {                                                \
    public:                                                                    \
        GTEST_TEST_CLASS_NAME_(test_case_name, test_name)() {}                 \
        const char* getTestCaseName() override { return #test_case_name; }     \
        const char* getTestName() override { return #test_name; }              \
                                                                               \
    private:                                                                   \
        virtual void TestBody();                                               \
        static ::testing::TestInfo* const test_info_ GTEST_ATTRIBUTE_UNUSED_;  \
        GTEST_DISALLOW_COPY_AND_ASSIGN_(GTEST_TEST_CLASS_NAME_(test_case_name, \
                                                               test_name));    \
    };                                                                         \
                                                                               \
    ::testing::TestInfo* const GTEST_TEST_CLASS_NAME_(test_case_name,          \
                                                      test_name)::test_info_ = \
            ::testing::internal::MakeAndRegisterTestInfo(                      \
                    #test_case_name, #test_name, NULL, NULL, (parent_id),      \
                    parent_class::SetUpTestCase,                               \
                    parent_class::TearDownTestCase,                            \
                    new ::testing::internal::TestFactoryImpl<                  \
                            GTEST_TEST_CLASS_NAME_(test_case_name,             \
                                                   test_name)>);               \
    void GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::TestBody()

namespace {
class FileLockTestInterface : public ::testing::Test {
public:
    virtual const char* getTestCaseName() { return "FileLockTest"; }
    virtual const char* getTestName() = 0;

protected:
    virtual bool isParentProcess() const = 0;
    virtual size_t writeToOther(void* buffer, size_t size) = 0;
    virtual size_t readFromOther(void* buffer, size_t size) = 0;
    std::unique_ptr<android::base::TestTempDir> mTempDir;
    std::string mFileLockPath;
};

#ifdef _WIN32
bool sIsParentProcess = true;
HANDLE sChildRead = nullptr;
HANDLE sChildWrite = nullptr;
char sFileLockPath[MAX_PATH] = {};

class FileLockTest : public FileLockTestInterface {
public:
    void SetUp() override {
        const char* kFileLockName = "filelock.txt";
        if (sIsParentProcess) {
            mTempDir.reset(new android::base::TestTempDir("FileLockTest"));
            mFileLockPath = mTempDir->makeSubPath(kFileLockName);
            ASSERT_TRUE(mTempDir->makeSubFile(kFileLockName));
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = true;

            ASSERT_TRUE(CreatePipe(&mChildRead, &mParentWrite, &sa, 0));
            ASSERT_TRUE(CreatePipe(&mParentRead, &mChildWrite, &sa, 0));

            // Get the name of executable
            char exePath[MAX_PATH];
            ASSERT_NE(GetModuleFileName(NULL, exePath, MAX_PATH), MAX_PATH);
            char cmdBuffer[MAX_PATH * 2 + 100];
            ASSERT_GT(sizeof(cmdBuffer),
                      snprintf(cmdBuffer, MAX_PATH,
                               "%s --gtest_filter=%s.%s --child --read=%p "
                               "--write=%p "
                               "--file-lock-path=\"%s\"",
                               exePath, getTestCaseName(), getTestName(),
                               mChildRead, mChildWrite, mFileLockPath.c_str()));
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset(&si, 0, sizeof(si));
            memset(&pi, 0, sizeof(pi));
            si.cb = sizeof(si);
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.dwFlags |= STARTF_USESTDHANDLES;
            ASSERT_TRUE(CreateProcess(nullptr, cmdBuffer, nullptr, nullptr,
                                      true, 0, nullptr, nullptr, &si, &pi));
        } else {
            mFileLockPath = sFileLockPath;
            mChildRead = sChildRead;
            mChildWrite = sChildWrite;
        }
    }
    void TearDown() override {
        CloseHandle(mChildRead);
        CloseHandle(mChildWrite);
        if (isParentProcess()) {
            CloseHandle(mParentRead);
            CloseHandle(mParentWrite);
        }
        if (!isParentProcess()) {
            exit(0);
        }
    }

protected:
    bool isParentProcess() const override { return sIsParentProcess; }
    size_t writeToOther(void* buffer, size_t size) override {
        HANDLE pipe = isParentProcess() ? mParentWrite : mChildWrite;
        DWORD ret = 0;
        WriteFile(pipe, buffer, size, &ret, nullptr);
        return ret;
    }
    size_t readFromOther(void* buffer, size_t size) override {
        HANDLE pipe = isParentProcess() ? mParentRead : mChildRead;
        DWORD ret = 0;
        ReadFile(pipe, buffer, size, &ret, nullptr);
        return ret;
    }
    HANDLE mChildRead;
    HANDLE mChildWrite;
    HANDLE mParentRead;
    HANDLE mParentWrite;
};

#else

class FileLockTest : public FileLockTestInterface {
public:
    void SetUp() override {
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
    void TearDown() override {
        close(mPipe0[0]);
        close(mPipe0[1]);
        close(mPipe1[0]);
        close(mPipe1[1]);
        if (!isParentProcess()) {
            exit(0);
        }
    }

protected:
    bool isParentProcess() const override { return mPid == 0; }
    size_t writeToOther(void* buffer, size_t size) override {
        int pipe = isParentProcess() ? mPipe0[1] : mPipe1[1];
        return write(pipe, buffer, size);
    }
    size_t readFromOther(void* buffer, size_t size) override {
        int pipe = isParentProcess() ? mPipe1[0] : mPipe0[0];
        return read(pipe, buffer, size);
    }
    int mPid = 0;
    int mPipe0[2] = {};
    int mPipe1[2] = {};
};
#endif
}

TEST_F_WITH_NAME(FileLockTest, createConflict) {
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

TEST_F_WITH_NAME(FileLockTest, createAndRelease) {
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

TEST_F_WITH_NAME(FileLockTest, waitForStale) {
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

int main(int argc, char** argv) {
#ifdef _WIN32
#define _READ_STR "--read"
#define _WRITE_STR "--write"
#define _FILE_PATH_STR "--file-lock-path"
    for (int i = 1; i < argc; i++) {
        if (!strcmp("--child", argv[i])) {
            sIsParentProcess = false;
        } else if (!strncmp(_READ_STR, argv[i], strlen(_READ_STR))) {
            sscanf(argv[i], _READ_STR "=%p", &sChildRead);
        } else if (!strncmp(_WRITE_STR, argv[i], strlen(_WRITE_STR))) {
            sscanf(argv[i], _WRITE_STR "=%p", &sChildWrite);
        } else if (!strncmp(_FILE_PATH_STR, argv[i], strlen(_FILE_PATH_STR))) {
            sscanf(argv[i], _FILE_PATH_STR "=%s", sFileLockPath);
        }
    }
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
