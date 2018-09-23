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

#include "android/offworld/OffworldPipe.h"
#include "android/base/ArraySize.h"
#include "android/base/files/MemStream.h"
#include "android/base/system/System.h"
#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/testing/TestVmLock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace pb = ::offworld;
using namespace android;
using namespace android::offworld;
using namespace android::base;

using ::testing::ElementsAreArray;

class OffworldPipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        base_enable_verbose_logs();
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        AndroidPipe::initThreadingForTest(TestVmLock::getInstance(),
                                          mLooper.get());

        registerOffworldPipeServiceForTest();
    }

    void TearDown() override {
        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
        // Reset initThreading to avoid using our looper.
        AndroidPipe::initThreading(TestVmLock::getInstance());
        mLooper.reset();
    }

    void writeMessage(void* pipe, const google::protobuf::Message& message) {
        const int size = message.ByteSize();
        std::vector<uint8_t> data(static_cast<size_t>(size));
        EXPECT_TRUE(message.SerializeToArray(data.data(), size));

        const uint32_t payloadSize = static_cast<uint32_t>(data.size());
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(pipe, &payloadSize, sizeof(uint32_t)));

        EXPECT_THAT(mDevice->write(pipe, data), IsOk());
    }

    template <typename T>
    T readMessage(void* pipe) {
        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(pipe, &responseSize, sizeof(uint32_t)));

        auto result = mDevice->read(pipe, responseSize);
        EXPECT_TRUE(result.ok());
        if (result.ok()) {
            std::vector<uint8_t> data = result.unwrap();

            T message;
            EXPECT_TRUE(message.ParseFromArray(data.data(), data.size()));
            return message;
        } else {
            return T();
        }
    }

    void snapshotSave(void* pipe, base::Stream* stream) {
        RecursiveScopedVmLock lock;
        auto cStream = reinterpret_cast<::Stream*>(stream);
        android_pipe_guest_pre_save(cStream);
        android_pipe_guest_save(pipe, cStream);
        android_pipe_guest_post_save(cStream);
    }

    void* snapshotLoad(base::Stream* stream) {
        RecursiveScopedVmLock lock;
        auto cStream = reinterpret_cast<::Stream*>(stream);
        android_pipe_guest_pre_load(cStream);
        void* pipe = mDevice->load(stream);
        EXPECT_NE(pipe, nullptr);
        android_pipe_guest_post_load(cStream);
        return pipe;
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    std::unique_ptr<base::TestLooper> mLooper;
};

TEST_F(OffworldPipeTest, Connect) {
    void* pipe = mDevice->connect("OffworldPipe");

    pb::ConnectHandshake connect;
    connect.set_version(android::offworld::kProtocolVersion);

    writeMessage(pipe, connect);

    auto response = readMessage<pb::ConnectHandshakeResponse>(pipe);
    EXPECT_EQ(response.result(), pb::ConnectHandshakeResponse::RESULT_NO_ERROR);

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, ConnectionFailure) {
    void* pipe = mDevice->connect("OffworldPipe");

    pb::ConnectHandshake connect;
    connect.set_version(UINT32_MAX);

    writeMessage(pipe, connect);

    auto response = readMessage<pb::ConnectHandshakeResponse>(pipe);
    EXPECT_EQ(response.result(),
              pb::ConnectHandshakeResponse::RESULT_ERROR_VERSION_MISMATCH);

    // TODO(jwmcglynn): Check that the pipe was closed by the host.

    mDevice->close(pipe);
}
