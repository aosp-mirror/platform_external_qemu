// Copyright (C) 2018 The Android Open Source Project
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

#include "android/emulation/RefcountPipe.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/MemStream.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestEvent.h"
#include "android/base/testing/TestLooper.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/refcount-pipe.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace android {

class RefcountPipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        AndroidPipe::initThreadingForTest(TestVmLock::getInstance(),
                                          mLooper.get());
    }

    void TearDown() override {
        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
        // Reset initThreading to avoid using our looper.
        AndroidPipe::initThreading(TestVmLock::getInstance());
        mLooper.reset();
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    std::unique_ptr<base::TestLooper> mLooper;
};

// Test closing of the refcount pipe will trigger the callback and the handle is
// passed down.
TEST_F(RefcountPipeTest, CB_HANDLE) {
    android_init_refcount_pipe();
    auto pipe = mDevice->connect("refcount");
    const uint32_t kCBHandle = 100;
    uint32_t expectedCBHandle = 0;

    auto onClose = [&](uint32_t handle) { expectedCBHandle = handle; };
    android::emulation::registerOnLastRefCallback(onClose);
    mDevice->write(pipe, &kCBHandle, sizeof(kCBHandle));
    EXPECT_EQ(0, expectedCBHandle);
    mDevice->close(pipe);
    EXPECT_EQ(kCBHandle, expectedCBHandle);
}

}  // namespace android
