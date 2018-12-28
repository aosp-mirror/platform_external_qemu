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

#include "android/emulation/HostMemoryService.h"

#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/testing/MockAndroidVmOperations.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/utils/looper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

using namespace android;
using namespace android::base;

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Mock;
using ::testing::StrEq;

static constexpr base::Looper::Duration kTimeoutMs = 15000;  // 15 seconds.

class HostMemoryServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        AndroidPipe::initThreadingForTest(TestVmLock::getInstance(),
                                          mLooper.get());
        // TestLooper -> base::Looper -> C Looper
        android_registerMainLooper(reinterpret_cast<::Looper*>(
                static_cast<base::Looper*>(mLooper.get())));

        android_host_memory_service_init();
    }

    void TearDown() override {
        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
        // Reset initThreading to avoid using our looper.
        AndroidPipe::initThreading(TestVmLock::getInstance());
        android_registerMainLooper(nullptr);
        mLooper.reset();
    }

    void* connectService() {
        return mDevice->connect("HostMemoryPipe");
    }

    void writeCmd(void* pipe, uint32_t cmd) {
        std::vector<uint8_t> payload(sizeof(uint32_t));
        memcpy(payload.data(), &cmd, sizeof(uint32_t));

        uint32_t bytes = (uint32_t)payload.size();
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(pipe, &bytes, sizeof(uint32_t)));
        EXPECT_THAT(mDevice->write(pipe, payload), IsOk());
    }

    void writeCmdWithData(void* pipe, uint32_t cmd, uint64_t addr, uint64_t size) {
        size_t totalSize = sizeof(uint32_t) + 2 * sizeof(uint64_t);

        std::vector<uint8_t> payload(totalSize);
        memcpy(payload.data(), &cmd, sizeof(uint32_t));
        memcpy(payload.data() + sizeof(uint32_t),
               &addr, sizeof(uint64_t));
        memcpy(payload.data() + sizeof(uint32_t) + sizeof(uint64_t),
               &size, sizeof(uint64_t));

        uint32_t bytes = (uint32_t)payload.size();
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(pipe, &bytes, sizeof(uint32_t)));
        EXPECT_THAT(mDevice->write(pipe, payload), IsOk());
    }

    void block(void* pipe) {
        const base::Looper::Duration deadline = mLooper->nowMs() + kTimeoutMs;

        // Since these unittests never expect long-running operations, run in
        // a loop with only a 1ms minimal sleep.
        while ((mDevice->poll(pipe) & PIPE_POLL_IN) == 0 &&
               mLooper->nowMs() < deadline) {
            mLooper->runOneIterationWithDeadlineMs(mLooper->nowMs() + 1);
        }
    }

    uint32_t readCmdResponseBlocking(void* pipe) {
        block(pipe);

        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(pipe, &responseSize, sizeof(uint32_t)));

        EXPECT_EQ(sizeof(uint32_t), responseSize);

        std::vector<uint8_t> response =
            mDevice->read(pipe, responseSize).unwrap();

        return *(uint32_t*)(response.data());
    }

    uint64_t readDataResponseBlocking(void* pipe) {
        block(pipe);

        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(pipe, &responseSize, sizeof(uint32_t)));
        EXPECT_EQ(sizeof(uint64_t), responseSize);

        std::vector<uint8_t> response =
            mDevice->read(pipe, responseSize).unwrap();

        return *(uint64_t*)(response.data());
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    std::unique_ptr<TestLooper> mLooper;
};

// Check that basic connection works.
TEST_F(HostMemoryServiceTest, Connect) {
    void* pipe = connectService();
    mDevice->close(pipe);
}

// Check that sending a null command does nothing.
TEST_F(HostMemoryServiceTest, NullCommand) {
    void* pipe = connectService();
    writeCmd(pipe, (uint32_t)HostMemoryServiceCommand::None);
    mDevice->close(pipe);
}

// Tests that if the shared region was not allocated yet,
// it returns false.
TEST_F(HostMemoryServiceTest, QuerySharedRegionAllocated) {
    void* pipe = connectService();
    writeCmd(pipe, (uint32_t)HostMemoryServiceCommand::IsSharedRegionAllocated);
    auto res = readCmdResponseBlocking(pipe);
    EXPECT_EQ(0, res);
    mDevice->close(pipe);
}

// Allocate the region.
TEST_F(HostMemoryServiceTest, AllocateSharedRegion) {
    void* pipe = connectService();
    writeCmdWithData(
        pipe,
        (uint32_t)HostMemoryServiceCommand::AllocSharedRegion,
        0xabcde000,
        0x1000);
    writeCmd(pipe, (uint32_t)HostMemoryServiceCommand::IsSharedRegionAllocated);
    auto res = readCmdResponseBlocking(pipe);
    EXPECT_NE(0, res);
    mDevice->close(pipe);
}

// Test alloc/free of subregions.
TEST_F(HostMemoryServiceTest, AllocFreeSubRegion) {
    void* pipe = connectService();
    writeCmdWithData(
        pipe,
        (uint32_t)HostMemoryServiceCommand::AllocSubRegion,
        0x0,
        0x100);
    writeCmdWithData(
        pipe,
        (uint32_t)HostMemoryServiceCommand::AllocSubRegion,
        0x0,
        0x100);
    mDevice->close(pipe);
}