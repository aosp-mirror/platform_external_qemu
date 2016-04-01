// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/vm_lock.h"

#include <gtest/gtest.h>

namespace {

struct TestData {
    int lockCount;
    int unlockCount;

    static void reset() {
        sInstance.lockCount = 0;
        sInstance.unlockCount = 0;

        android_vm_set_lock_funcs(&TestData::onLock, &TestData::onUnlock);
    }

    static void onLock(void) {
        sInstance.lockCount++;
    }

    static void onUnlock(void) {
        sInstance.unlockCount++;
    }

    static TestData sInstance;
};

// static
TestData TestData::sInstance = {};

}  // namespace

TEST(vm_lock, Default) {
    EXPECT_EQ(0, TestData::sInstance.lockCount);
    EXPECT_EQ(0, TestData::sInstance.unlockCount);

    android_vm_lock();
    EXPECT_EQ(0, TestData::sInstance.lockCount);
    EXPECT_EQ(0, TestData::sInstance.unlockCount);

    android_vm_unlock();
    EXPECT_EQ(0, TestData::sInstance.lockCount);
    EXPECT_EQ(0, TestData::sInstance.unlockCount);
}

TEST(vm_lock, CustomLockUnlock) {
    EXPECT_EQ(0, TestData::sInstance.lockCount);
    EXPECT_EQ(0, TestData::sInstance.unlockCount);

    TestData::reset();

    android_vm_lock();
    EXPECT_EQ(1, TestData::sInstance.lockCount);
    EXPECT_EQ(0, TestData::sInstance.unlockCount);

    android_vm_unlock();
    EXPECT_EQ(1, TestData::sInstance.lockCount);
    EXPECT_EQ(1, TestData::sInstance.unlockCount);

    android_vm_lock();
    EXPECT_EQ(2, TestData::sInstance.lockCount);
    EXPECT_EQ(1, TestData::sInstance.unlockCount);

    android_vm_unlock();
    EXPECT_EQ(2, TestData::sInstance.lockCount);
    EXPECT_EQ(2, TestData::sInstance.unlockCount);

    android_vm_set_lock_funcs(nullptr, nullptr);

    android_vm_unlock();
    EXPECT_EQ(2, TestData::sInstance.lockCount);
    EXPECT_EQ(2, TestData::sInstance.unlockCount);

    android_vm_lock();
    EXPECT_EQ(2, TestData::sInstance.lockCount);
    EXPECT_EQ(2, TestData::sInstance.unlockCount);
}
