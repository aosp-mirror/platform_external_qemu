// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/memory/SharedMemory.h"

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "android/base/testing/TestSystem.h"

namespace android {
namespace base {

TEST(SharedMemory, ShareVisibileWithinSameProc) {
    const mode_t user_read_only = 0600;
    std::string unique_name = "tst_21654869810548";
    std::string message = "Hello World!";
    base::SharedMemory mWriter(unique_name, message.size());
    base::SharedMemory mReader(unique_name, message.size());

    ASSERT_FALSE(mWriter.isOpen());
    ASSERT_FALSE(mReader.isOpen());

    int err = mWriter.create(user_read_only);
    ASSERT_EQ(0, err);
    err = mReader.open(SharedMemory::AccessMode::READ_ONLY);
    ASSERT_EQ(0, err);

    ASSERT_TRUE(mWriter.isOpen());
    ASSERT_TRUE(mReader.isOpen());

    memcpy(*mWriter, message.c_str(), message.size());
    std::string read(static_cast<const char*>(*mReader));
    ASSERT_TRUE(message == read);

    mWriter.close();
    mReader.close();
    ASSERT_FALSE(mWriter.isOpen());
    ASSERT_FALSE(mReader.isOpen());
}

TEST(SharedMemory, ShareFileBackedVisibileWithinSameProc) {
    android::base::TestSystem ts("/home", 64);
    // Note the unicode character in the filename below!!
    std::string unique_name = android::base::PathUtils::join(
            ts.getTempRoot()->path(), "shāred.mem");
    const mode_t user_read_only = 0600;
    std::string message = "Hello World!";
    base::SharedMemory mWriter("file://" + unique_name, message.size());
    base::SharedMemory mReader("file://" + unique_name, message.size());

    ASSERT_FALSE(mWriter.isOpen());
    ASSERT_FALSE(mReader.isOpen());

    int err = mWriter.create(user_read_only);
    ASSERT_EQ(0, err);
    err = mReader.open(SharedMemory::AccessMode::READ_ONLY);
    ASSERT_EQ(0, err);

    ASSERT_TRUE(mWriter.isOpen());
    ASSERT_TRUE(mReader.isOpen());

    memcpy(*mWriter, message.c_str(), message.size());
    std::string read(static_cast<const char*>(*mReader));

    EXPECT_EQ(message, read);

    mWriter.close();
    mReader.close();
    ASSERT_FALSE(mWriter.isOpen());
    ASSERT_FALSE(mReader.isOpen());
}

TEST(SharedMemory, ShareFileCanReadAfterDelete) {
    // Make sure you can still read the memory, even if the server has marked
    // the file for deletion.
    android::base::TestSystem ts("/home", 64);
    // Note the unicode character in the filename below!!
    std::string unique_name = android::base::PathUtils::join(
            ts.getTempRoot()->path(), "shāred.mem");
    const mode_t user_read_only = 0600;
    std::string message = "Hello World!";
    base::SharedMemory mWriter("file://" + unique_name, message.size());
    base::SharedMemory mReader("file://" + unique_name, message.size());

    ASSERT_FALSE(mWriter.isOpen());
    ASSERT_FALSE(mReader.isOpen());

    mWriter.create(user_read_only);
    memcpy(*mWriter, message.c_str(), message.size());

    mReader.open(SharedMemory::AccessMode::READ_ONLY);
    mWriter.close();

    std::string read(static_cast<const char*>(*mReader));
    ASSERT_TRUE(message == read);

    mReader.close();
}

TEST(SharedMemory, ShareFileDoesCleanedUp) {
    // Make sure that the file gets removed after the server closes down.
    android::base::TestSystem ts("/home", 64);
    std::string unique_name = android::base::PathUtils::join(
            ts.getTempRoot()->path(), "shared.mem");
    const mode_t user_read_only = 0600;
    std::string message = "Hello World!";
    base::SharedMemory mWriter("file://" + unique_name, message.size());

    ASSERT_FALSE(mWriter.isOpen());
    mWriter.create(user_read_only);
    memcpy(*mWriter, message.c_str(), message.size());
    ASSERT_TRUE(ts.host()->pathExists(unique_name));
    mWriter.close();
    ASSERT_FALSE(ts.host()->pathExists(unique_name));
}

TEST(SharedMemory, CanShare4KVideo) {
    // Make sure we can use this for sharing video, for now 4K ought to be
    // enough for anybody.
    // This test will likely segfault/fail on a default MacOS config when using
    // shared memory.
    //
    // See:  sysctl -A | grep shm
    // which should produce something like:
    // kern.sysv.shmmax: 4194304
    // kern.sysv.shmmin: 1
    // kern.sysv.shmmni: 32
    // kern.sysv.shmseg: 8
    // kern.sysv.shmall: 1024

    const int FourK = 3840 * 2160 * 4; // 4k resolution with 4 bytes per pixel.
    android::base::TestSystem ts("/home", FourK);
    std::string unique_name = android::base::PathUtils::join(
            ts.getTempRoot()->path(), "shared.mem");
    const mode_t user_read_only = 0600;
    std::string message = "Hello World!";
    base::SharedMemory mWriter("file://" + unique_name, message.size());

    ASSERT_FALSE(mWriter.isOpen());
    mWriter.create(user_read_only);
    memcpy(*mWriter, message.c_str(), message.size());
    ASSERT_TRUE(ts.host()->pathExists(unique_name));
    mWriter.close();
    ASSERT_FALSE(ts.host()->pathExists(unique_name));
}

TEST(SharedMemory, CannotOpenTwice) {
    const mode_t user_read_only = 0600;
    std::string unique_name = "tst_21654869810548";
    std::string message = "Hello World!";
    base::SharedMemory mWriter(unique_name, message.size());
    base::SharedMemory mReader(unique_name, message.size());
    int err = mWriter.create(user_read_only);
    ASSERT_EQ(0, err);
    err = mReader.open(SharedMemory::AccessMode::READ_ONLY);
    ASSERT_EQ(0, err);

    // Second create should fail..
    err = mWriter.create(user_read_only);
    ASSERT_NE(0, err);

    // Second open should fail..
    err = mReader.open(SharedMemory::AccessMode::READ_ONLY);
    ASSERT_NE(0, err);
}

TEST(SharedMemory, CreateNoMapping) {
    const mode_t user_read_write = 0755;
    std::string name = "tst_21654869810548";
    base::SharedMemory mem(name, 256);

    ASSERT_FALSE(mem.isOpen());

    int err = mem.createNoMapping(user_read_write);
    ASSERT_EQ(0, err);

    ASSERT_FALSE(mem.isMapped());

    mem.close();
    ASSERT_FALSE(mem.isOpen());
}

}  // namespace base
}  // namespace android
