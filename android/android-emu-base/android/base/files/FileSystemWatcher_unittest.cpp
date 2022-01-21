// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/FileSystemWatcher.h"

#include <gtest/gtest.h>  // for Message, TestPartResult
#include <fstream>        // for endl
#include <limits>         // for numeric_limits
#include <memory>         // for std::unique_ptr
#include <string>         // for string
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector

#include <iostream>
#include "android/base/system/System.h"
#include "android/base/testing/TestEvent.h"    // for TestEvent
#include "android/base/testing/TestTempDir.h"  // for TestTempDir
#include "gtest/gtest.h"                       // for AssertionResult, EXPEC...

#include "android/base/files/PathUtils.h"
namespace android {
namespace base {
using android::base::pj;

namespace {

class FileSystemWatcherTest : public ::testing::Test {
public:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("watcher_test"));
        mTestEv.reset();
    }

    void TearDown() override {
        mTempDir.reset();
        mTestEv.reset();
    }

protected:
    std::unique_ptr<TestTempDir> mTempDir;
    TestEvent mTestEv;
};

}  // namespace

TEST_F(FileSystemWatcherTest, file_add) {
    auto watcher = FileSystemWatcher::getFileSystemWatcher(
            mTempDir->path(), [=](auto change, auto path) {
                EXPECT_EQ(change,
                          FileSystemWatcher::WatcherChangeType::Created);
                EXPECT_EQ(path, pj(mTempDir->path(), "hello.txt"));
                mTestEv.signal();
            });
    EXPECT_TRUE(watcher->start());
    ASSERT_TRUE(mTempDir->makeSubFile("hello.txt"));
    mTestEv.wait();
}

TEST_F(FileSystemWatcherTest, dir_add) {
    auto watcher = FileSystemWatcher::getFileSystemWatcher(
            mTempDir->path(), [=](auto change, auto path) {
                EXPECT_EQ(change,
                          FileSystemWatcher::WatcherChangeType::Created);
                EXPECT_EQ(path, pj(mTempDir->path(), "hello"));
                mTestEv.signal();
            });
    EXPECT_TRUE(watcher->start());
    ASSERT_TRUE(mTempDir->makeSubDir("hello"));
    mTestEv.wait();
}

TEST_F(FileSystemWatcherTest, file_remove) {
    ASSERT_TRUE(mTempDir->makeSubFile("hello.txt"));
    auto watcher = FileSystemWatcher::getFileSystemWatcher(
            mTempDir->path(), [=](auto change, auto path) {
                EXPECT_EQ(change,
                          FileSystemWatcher::WatcherChangeType::Deleted);
                EXPECT_EQ(path, pj(mTempDir->path(), "hello.txt"));
                mTestEv.signal();
            });
    EXPECT_TRUE(watcher->start());
    System::get()->deleteFile(pj(mTempDir->path(), "hello.txt"));
    mTestEv.wait();
}

TEST_F(FileSystemWatcherTest, file_change) {
    ASSERT_TRUE(mTempDir->makeSubFile("hello.txt"));
    auto fname = mTempDir->path();
    auto watcher = FileSystemWatcher::getFileSystemWatcher(
            fname, [=](auto change, auto path) {
                EXPECT_EQ(change,
                          FileSystemWatcher::WatcherChangeType::Changed);
                EXPECT_EQ(path, pj(fname, "hello.txt"));
                mTestEv.signal();
            });
    EXPECT_TRUE(watcher->start());
    std::ofstream myfile;
    myfile.open(pj(fname, "hello.txt"));
    myfile << "Writing this to a file.\n";
    myfile.close();
    mTestEv.wait();
}

TEST_F(FileSystemWatcherTest, file_add_unicode) {
    auto watcher = FileSystemWatcher::getFileSystemWatcher(
            mTempDir->path(), [=](auto change, auto path) {
                EXPECT_EQ(change,
                          FileSystemWatcher::WatcherChangeType::Created);
                EXPECT_EQ(path, pj(mTempDir->path(), "hello☺.txt"));
                mTestEv.signal();
            });
    EXPECT_TRUE(watcher->start());
    ASSERT_TRUE(mTempDir->makeSubFile("hello☺.txt"));
    mTestEv.wait();
}

}  // namespace base
}  // namespace android
