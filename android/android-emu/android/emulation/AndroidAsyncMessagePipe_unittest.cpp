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
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/MemStream.h"
#include "android/base/system/System.h"
#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestEvent.h"
#include "android/base/testing/TestLooper.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/hostdevices/HostGoldfishPipe.h"
#include "android/emulation/testing/TestVmLock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

using android::base::arraySize;
using android::base::IsErr;
using android::base::IsOk;
using testing::ElementsAreArray;
using testing::ElementsAre;
using testing::StrEq;

namespace android {

class AndroidAsyncMessagePipeTest : public ::testing::Test {
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

    void writePacket(int pipe, const std::vector<uint8_t>& data) {
        const uint32_t payloadSize = static_cast<uint32_t>(data.size());
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(pipe, &payloadSize, sizeof(uint32_t)));

        EXPECT_THAT(mDevice->write(pipe, data), IsOk());
    }

    std::vector<uint8_t> readPacket(int pipe) {
        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(pipe, &responseSize, sizeof(uint32_t)));

        auto result = mDevice->read(pipe, responseSize);
        EXPECT_TRUE(result.ok());
        if (result.ok()) {
            std::vector<uint8_t> data = result.unwrap();
            EXPECT_EQ(data.size(), responseSize);
            return data;
        } else {
            return std::vector<uint8_t>();
        }
    }

    void pumpLooperUntilEvent(TestEvent& event) {
        constexpr base::Looper::Duration kTimeoutMs = 15000;  // 15 seconds.
        constexpr base::Looper::Duration kStep = 50;          // 50 ms.

        base::Looper::Duration current = mLooper->nowMs();
        const base::Looper::Duration deadline = mLooper->nowMs() + kTimeoutMs;

        while (!event.isSignaled() && current + kStep < deadline) {
            mLooper->runOneIterationWithDeadlineMs(current + kStep);
            current = mLooper->nowMs();
        }

        event.wait();
    }

    void snapshotSave(int pipe, base::Stream* stream) {
        RecursiveScopedVmLock lock;
        mDevice->saveSnapshot(stream, pipe);
    }

    int snapshotLoad(base::Stream* stream) {
        RecursiveScopedVmLock lock;
        int pipe = mDevice->loadSnapshotSinglePipe(stream);
        EXPECT_GE(pipe, 0);
        return pipe;
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    std::unique_ptr<base::TestLooper> mLooper;
};

constexpr uint8_t kSimplePayload[] = "Hello World";
constexpr uint8_t kSimpleResponse[] = "response";

class SimpleMessagePipe : public AndroidAsyncMessagePipe {
public:
    SimpleMessagePipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

    void onMessage(const std::vector<uint8_t>& data) override {
        EXPECT_THAT(data, ElementsAreArray(kSimplePayload));

        const size_t bytes = arraySize(kSimpleResponse);
        std::vector<uint8_t> output(kSimpleResponse, kSimpleResponse + bytes);
        send(std::move(output));
    }
};

TEST_F(AndroidAsyncMessagePipeTest, Basic) {
    registerAsyncMessagePipeService(
        std::make_unique<AndroidAsyncMessagePipe::Service<SimpleMessagePipe>>(
                    "TestPipe"));

    auto pipe = mDevice->connect("TestPipe");

    const uint32_t payloadSize = arraySize(kSimplePayload);
    EXPECT_EQ(sizeof(uint32_t),
              mDevice->write(pipe, &payloadSize, sizeof(uint32_t)));

    std::vector<uint8_t> toSend(std::begin(kSimplePayload),
                                std::end(kSimplePayload));
    EXPECT_THAT(mDevice->write(pipe, toSend), IsOk());

    uint32_t responseSize = 0;
    EXPECT_EQ(sizeof(uint32_t),
              mDevice->read(pipe, (void*)&responseSize, sizeof(uint32_t)));
    EXPECT_EQ(responseSize, arraySize(kSimpleResponse));

    EXPECT_THAT(mDevice->read(pipe, responseSize),
                IsOk(ElementsAreArray(kSimpleResponse)));
    mDevice->close(pipe);
}

TEST_F(AndroidAsyncMessagePipeTest, Lambda) {
    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        EXPECT_THAT(data, ElementsAreArray(kSimplePayload));

        const size_t bytes = arraySize(kSimpleResponse);
        std::vector<uint8_t> output(kSimpleResponse, kSimpleResponse + bytes);
        sendCallback(std::move(output));
    };

    registerAsyncMessagePipeService("TestPipeLambda", onMessage);
    auto pipe = mDevice->connect("TestPipeLambda");

    const uint32_t payloadSize = arraySize(kSimplePayload);
    EXPECT_EQ(sizeof(uint32_t),
              mDevice->write(pipe, &payloadSize, sizeof(uint32_t)));

    std::vector<uint8_t> toSend(std::begin(kSimplePayload),
                                std::end(kSimplePayload));
    EXPECT_THAT(mDevice->write(pipe, toSend), IsOk());

    uint32_t responseSize = 0;
    EXPECT_EQ(sizeof(uint32_t),
              mDevice->read(pipe, (void*)&responseSize, sizeof(uint32_t)));
    EXPECT_EQ(responseSize, arraySize(kSimpleResponse));

    EXPECT_THAT(mDevice->read(pipe, responseSize),
                IsOk(ElementsAreArray(kSimpleResponse)));
    mDevice->close(pipe);
}

TEST_F(AndroidAsyncMessagePipeTest, OutOfBand) {
    std::vector<uint8_t> lastMessage;
    PipeSendFunction performSend;

    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        lastMessage = data;
        performSend = sendCallback;
    };

    registerAsyncMessagePipeService("OutOfBand", onMessage);
    auto pipe = mDevice->connect("OutOfBand");

    writePacket(pipe, {1, 2, 3});
    EXPECT_THAT(lastMessage, ElementsAre(1, 2, 3));
    EXPECT_EQ(mDevice->poll(pipe), PIPE_POLL_OUT);

    const std::vector<uint8_t> kResponse = {5, 6, 7};
    performSend(std::vector<uint8_t>(kResponse));
    EXPECT_EQ(mDevice->poll(pipe), PIPE_POLL_IN);
    EXPECT_THAT(readPacket(pipe), ElementsAreArray(kResponse));

    // Now try sending a second packet.
    EXPECT_EQ(mDevice->poll(pipe), PIPE_POLL_OUT);

    const std::vector<uint8_t> kResponse2 = {8, 9, 10, 11, 12};
    performSend(std::vector<uint8_t>(kResponse2));
    EXPECT_EQ(mDevice->poll(pipe), PIPE_POLL_IN);
    EXPECT_THAT(readPacket(pipe), ElementsAreArray(kResponse2));

    mDevice->close(pipe);
}

class CloseOnMessagePipe : public AndroidAsyncMessagePipe {
public:
    static constexpr uint8_t kCloseNow = 0;
    static constexpr uint8_t kQueueClose = 1;

    CloseOnMessagePipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

    void onMessage(const std::vector<uint8_t>& data) override {
        EXPECT_EQ(data.size(), 1);
        if (data[0] == kCloseNow) {
            send({4, 5, 6});  // Should never be received.
            closeFromHost();
        } else if (data[0] == kQueueClose) {
            send({1, 2, 3});
            queueCloseFromHost();
        } else {
            FAIL() << "Unexpected message: " << data[0];
        }
    }
};

// Attempt to close the pipe in the onMessage callback.
TEST_F(AndroidAsyncMessagePipeTest, CloseOnMessage) {
    registerAsyncMessagePipeService(
        std::make_unique<AndroidAsyncMessagePipe::Service<CloseOnMessagePipe>>(
            "ClosePipe"));

    auto pipe = mDevice->connect("ClosePipe");

    TestEvent receivedClose;
    mDevice->setWakeCallback(pipe, [&](int wakes) {
        if (wakes & PIPE_WAKE_CLOSED) {
            receivedClose.signal();
        }
    });

    writePacket(pipe, {CloseOnMessagePipe::kCloseNow});
    // Check that the pipe was closed by the host.
    pumpLooperUntilEvent(receivedClose);

    EXPECT_THAT(mDevice->read(pipe, 3), IsErr(EINVAL));

    mDevice->close(pipe);
}

// Verify closing the pipe skips processing future messages, with multiple
// simultaneous messages.
TEST_F(AndroidAsyncMessagePipeTest, CloseWithQueuedMessages) {
    registerAsyncMessagePipeService(
        std::make_unique<AndroidAsyncMessagePipe::Service<CloseOnMessagePipe>>(
            "ClosePipe"));

    auto pipe = mDevice->connect("ClosePipe");

    TestEvent receivedClose;
    mDevice->setWakeCallback(pipe, [&](int wakes) {
        if (wakes & PIPE_WAKE_CLOSED) {
            receivedClose.signal();
        }
    });

    // We need to manually craft the message, since we want all messages to
    // arrive in the same readBuffers call.
    std::vector<uint8_t> message((sizeof(uint32_t) + 1) * 2);
    // Packet 1.
    *reinterpret_cast<uint32_t*>(&message[0]) = 1;
    message[4] = CloseOnMessagePipe::kCloseNow;
    // Packet 2, should never process.  If it does CloseOnMessagePipe will hit
    // a FAIL() for unexpected message type.
    *reinterpret_cast<uint32_t*>(&message[5]) = 1;
    message[9] = 123;

    EXPECT_THAT(mDevice->write(pipe, message), IsOk(5));

    // Check that the pipe was closed by the host.
    pumpLooperUntilEvent(receivedClose);

    mDevice->close(pipe);
}

TEST_F(AndroidAsyncMessagePipeTest, QueueCloseOnMessage) {
    registerAsyncMessagePipeService(
        std::make_unique<AndroidAsyncMessagePipe::Service<CloseOnMessagePipe>>(
            "ClosePipe"));

    auto pipe = mDevice->connect("ClosePipe");

    TestEvent receivedClose;
    mDevice->setWakeCallback(pipe, [&](int wakes) {
        if (wakes & PIPE_WAKE_CLOSED) {
            receivedClose.signal();
        }
    });

    writePacket(pipe, {CloseOnMessagePipe::kQueueClose});

    EXPECT_THAT(readPacket(pipe), ElementsAre(1, 2, 3));

    // Check that the pipe was closed by the host.
    pumpLooperUntilEvent(receivedClose);

    mDevice->close(pipe);
}

class MultithreadedEchoMessagePipe : public AndroidAsyncMessagePipe {
public:
    MultithreadedEchoMessagePipe(AndroidPipe::Service* service,
                                 PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)),
          mWorker(&MultithreadedEchoMessagePipe::threadMain, this) {}
    ~MultithreadedEchoMessagePipe() {
        mQuit = true;
        mWorker.join();
    }

    void onMessage(const std::vector<uint8_t>& data) override {
        base::AutoLock lock(mLock);
        mMessages.push_back(data);
    }

private:
    void threadMain() {
        while (!mQuit) {
            base::System::get()->sleepMs(100);
            {
                base::AutoLock lock(mLock);
                if (!mMessages.empty()) {
                    send(std::move(mMessages.front()));
                    mMessages.pop_front();
                }
            }
        }
    }

    base::Lock mLock;
    std::deque<std::vector<uint8_t>> mMessages;
    std::atomic<bool> mQuit{false};
    std::thread mWorker;
};

// bug: 118512307
TEST_F(AndroidAsyncMessagePipeTest, DISABLED_Multithreaded) {
    registerAsyncMessagePipeService(
        std::make_unique<AndroidAsyncMessagePipe::Service<MultithreadedEchoMessagePipe>>(
            "Multithreaded"));

    auto pipe = mDevice->connect("Multithreaded");

    std::deque<std::vector<uint8_t>> packets;
    TestEvent event;

    mDevice->setWakeCallback(pipe, [&](int wakes) {
        EXPECT_EQ(wakes, PIPE_WAKE_READ);
        EXPECT_THAT(readPacket(pipe), ElementsAreArray(packets.front()));
        packets.pop_front();
        event.signal();
    });

    auto sendData = [&](std::vector<uint8_t> data) {
        {
            // Wake callback is called under the VM lock, protect packets with
            // it so we can safely access it inside the callback.
            RecursiveScopedVmLock lock;
            packets.push_back(data);
        }
        writePacket(pipe, std::move(data));
    };

    sendData({1, 2, 3});
    pumpLooperUntilEvent(event);

    sendData({2, 3, 4});
    sendData({5, 6, 7});
    pumpLooperUntilEvent(event);
    pumpLooperUntilEvent(event);

    mDevice->close(pipe);
}

TEST_F(AndroidAsyncMessagePipeTest, SendAfterDestroy) {
    std::vector<uint8_t> lastMessage;
    PipeSendFunction performSend;

    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        lastMessage = data;
        performSend = sendCallback;
    };

    registerAsyncMessagePipeService("AfterDestroy", onMessage);
    auto pipe = mDevice->connect("AfterDestroy");

    writePacket(pipe, {1, 2, 3});
    EXPECT_THAT(lastMessage, ElementsAre(1, 2, 3));
    mDevice->close(pipe);

    const std::vector<uint8_t> kResponse = {5, 6, 7};
    performSend(std::vector<uint8_t>(kResponse));
}

TEST_F(AndroidAsyncMessagePipeTest, Snapshot) {
    std::vector<uint8_t> lastMessage;
    PipeSendFunction performSend;

    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        lastMessage = data;
        performSend = sendCallback;
    };

    registerAsyncMessagePipeService("Snapshot", onMessage);
    auto pipe = mDevice->connect("Snapshot");

    writePacket(pipe, {1, 2, 3});
    EXPECT_THAT(lastMessage, ElementsAre(1, 2, 3));

    const std::vector<uint8_t> kResponse = {5, 6, 7};
    performSend(std::vector<uint8_t>(kResponse));

    base::MemStream snapshotStream;
    snapshotSave(pipe, &snapshotStream);
    mDevice->close(pipe);

    auto restoredPipe = snapshotLoad(&snapshotStream);
    EXPECT_EQ(mDevice->poll(restoredPipe), PIPE_POLL_IN);
    EXPECT_THAT(readPacket(restoredPipe), ElementsAreArray(kResponse));
}

// Verifies that getPipe can restore the pipe after snapshot load.
TEST_F(AndroidAsyncMessagePipeTest, SnapshotGetPipe) {
    typedef AndroidAsyncMessagePipe::Service<SimpleMessagePipe> PipeService;
    auto pipeService = new PipeService("TestPipe");
    registerAsyncMessagePipeService(std::unique_ptr<PipeService>(pipeService));

    auto pipe = mDevice->connect("TestPipe");

    AsyncMessagePipeHandle handle = static_cast<AndroidAsyncMessagePipe*>(
                                            static_cast<AndroidPipe*>(mDevice->getHostPipe(pipe)))
                                            ->getHandle();

    base::MemStream snapshotStream;
    snapshotSave(pipe, &snapshotStream);
    mDevice->close(pipe);

    auto restoredPipe = mDevice->getHostPipe(snapshotLoad(&snapshotStream));
    SimpleMessagePipe* derivedRestoredPipe = static_cast<SimpleMessagePipe*>(
            static_cast<AndroidPipe*>(restoredPipe));

    auto pipeRefOpt = pipeService->getPipe(handle);
    ASSERT_TRUE(pipeRefOpt.hasValue());
    EXPECT_EQ(pipeRefOpt.value().pipe, derivedRestoredPipe);
}

// Verifies that sendCallback remains valid and works after snapshot load.
TEST_F(AndroidAsyncMessagePipeTest, SnapshotSendCallback) {
    std::vector<uint8_t> lastMessage;
    PipeSendFunction performSend;

    auto onMessage = [&](const std::vector<uint8_t>& data,
                         PipeSendFunction sendCallback) {
        lastMessage = data;
        performSend = sendCallback;
    };

    registerAsyncMessagePipeService("SnapshotSend", onMessage);
    auto pipe = mDevice->connect("SnapshotSend");

    writePacket(pipe, {1, 2, 3});
    EXPECT_THAT(lastMessage, ElementsAre(1, 2, 3));

    base::MemStream snapshotStream;
    snapshotSave(pipe, &snapshotStream);
    mDevice->close(pipe);

    auto restoredPipe = snapshotLoad(&snapshotStream);
    const std::vector<uint8_t> kResponse = {5, 6, 7};
    performSend(std::vector<uint8_t>(kResponse));
    EXPECT_THAT(readPacket(restoredPipe), ElementsAreArray(kResponse));
}

}  // namespace android
