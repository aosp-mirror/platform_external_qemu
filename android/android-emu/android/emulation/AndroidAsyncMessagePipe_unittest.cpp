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

#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/base/ArraySize.h"
#include "android/base/testing/ResultMatchers.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using android::base::arraySize;
using android::base::IsOk;
using testing::ElementsAreArray;
using testing::StrEq;

namespace android {

class AndroidAsyncMessagePipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        base_enable_verbose_logs();
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
    }

    void TearDown() override {
        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
        base_disable_verbose_logs();
    }
    HostGoldfishPipeDevice* mDevice = nullptr;
};

constexpr uint8_t kPayload[] = "Hello World";
constexpr uint8_t kResponse[] = "response";

class SimpleMessagePipe : public AndroidAsyncMessagePipe {
public:
    SimpleMessagePipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

    void onMessage(const std::vector<uint8_t>& data) override {
        EXPECT_THAT(data, ElementsAreArray(kPayload));

        const size_t bytes = arraySize(kResponse);
        std::vector<uint8_t> output(kResponse, kResponse + bytes);
        send(std::move(output));
    }
};

TEST_F(AndroidAsyncMessagePipeTest, Basic) {
    AndroidPipe::Service::add(
            new AndroidAsyncMessagePipe::Service<SimpleMessagePipe>(
                    "TestPipe"));

    auto pipe = mDevice->connect("TestPipe");

    const int32_t payloadSz = arraySize(kPayload);
    EXPECT_EQ(sizeof(int32_t),
              mDevice->write(pipe, &payloadSz, sizeof(int32_t)));

    std::vector<uint8_t> toSend(std::begin(kPayload), std::end(kPayload));
    EXPECT_THAT(mDevice->write(pipe, toSend), IsOk());

    int32_t responseSize = 0;
    EXPECT_EQ(sizeof(int32_t),
              mDevice->read(pipe, (void*)&responseSize, sizeof(int32_t)));
    EXPECT_EQ(responseSize, arraySize(kResponse));

    EXPECT_THAT(mDevice->read(pipe, responseSize),
                IsOk(ElementsAreArray(kResponse)));
    mDevice->close(pipe);
}

TEST_F(AndroidAsyncMessagePipeTest, Lambda) {
    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        EXPECT_THAT(data, ElementsAreArray(kPayload));

        const size_t bytes = arraySize(kResponse);
        std::vector<uint8_t> output(kResponse, kResponse + bytes);
        sendCallback(std::move(output));
    };

    registerSimpleAndroidAsyncMessagePipeService("TestPipeLambda", onMessage);
    auto pipe = mDevice->connect("TestPipeLambda");

    const int32_t payloadSz = arraySize(kPayload);
    EXPECT_EQ(sizeof(int32_t),
              mDevice->write(pipe, &payloadSz, sizeof(int32_t)));

    std::vector<uint8_t> toSend(std::begin(kPayload), std::end(kPayload));
    EXPECT_THAT(mDevice->write(pipe, toSend), IsOk());

    int32_t responseSize = 0;
    EXPECT_EQ(sizeof(int32_t),
              mDevice->read(pipe, (void*)&responseSize, sizeof(int32_t)));
    EXPECT_EQ(responseSize, arraySize(kResponse));

    EXPECT_THAT(mDevice->read(pipe, responseSize),
                IsOk(ElementsAreArray(kResponse)));
    mDevice->close(pipe);
}

}  // namespace android
