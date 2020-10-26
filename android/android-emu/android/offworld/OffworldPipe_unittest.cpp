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

#include <android/console.h>                                    // for getCo...
#include <gmock/gmock.h>                                        // for Anyth...
#include <gtest/gtest.h>                                        // for Message
#include <stddef.h>                                             // for size_t
#include <cstdint>                                              // for uint8_t
#include <functional>                                           // for __base
#include <memory>                                               // for uniqu...
#include <string>                                               // for string
#include <type_traits>                                          // for decay...
#include <utility>                                              // for move
#include <vector>                                               // for vector

#include "android/automation/AutomationController.h"            // for Autom...
#include "android/base/Log.h"                                   // for android
#include "android/base/Result.h"                                // for Result
#include "android/base/async/Looper.h"                          // for Looper
#include "android/base/files/MemStream.h"                       // for MemSt...
#include "android/base/testing/MockUtils.h"                     // for repla...
#include "android/base/testing/ProtobufMatchers.h"              // for Equal...
#include "android/base/testing/ResultMatchers.h"                // for IsOk
#include "android/base/testing/TestEvent.h"                     // for TestE...
#include "android/base/testing/TestLooper.h"                    // for TestL...
#include "android/emulation/AndroidAsyncMessagePipe.h"          // for Async...
#include "android/emulation/AndroidPipe.h"                      // for Andro...
#include "android/emulation/VmLock.h"                           // for Recur...
#include "android/emulation/android_pipe_common.h"              // for PIPE_...
#include "android/emulation/control/callbacks.h"                // for LineC...
#include "android/emulation/hostdevices/HostGoldfishPipe.h"     // for HostG...
#include "android/emulation/testing/MockAndroidVmOperations.h"  // for MockA...
#include "android/emulation/testing/TestVmLock.h"               // for TestV...
#include "android/globals.h"                                    // for engin...
#include "android/opengl/emugl_config.h"                        // for emugl...
#include "android/physics/PhysicalModel.h"                      // for physi...
#include "android/physics/Physics.h"                            // for PARAM...
#include "android/skin/winsys.h"                                // for WINSY...
#include "android/snapshot/SnapshotAPI.h"                       // for onOff...
#include "android/snapshot/interface.h"                         // for andro...
#include "android/utils/looper.h"                               // for andro...
#include "android/videoinjection/VideoInjectionController.h"    // for Video...
#include "google/protobuf/message.h"                            // for Message
#include "google/protobuf/text_format.h"                        // for TextF...
#include "offworld.pb.h"                                        // for Response

namespace android {
namespace base {
class Stream;
}  // namespace base
}  // namespace android


using namespace android;
using namespace android::offworld;
using namespace android::base;
using android::automation::AutomationController;
using android::videoinjection::VideoInjectionController;

using android::EqualsProto;
using android::proto::Partially;
using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Mock;
using ::testing::StrEq;

static constexpr base::Looper::Duration kTimeoutMs = 15000;  // 15 seconds.

constexpr uint64_t secondsToNs(float seconds) {
    return static_cast<uint64_t>(seconds * 1000000000.0);
}

struct PhysicalModelDeleter {
    void operator()(PhysicalModel* physicalModel) {
        physicalModel_free(physicalModel);
    }
};

typedef std::unique_ptr<PhysicalModel, PhysicalModelDeleter> PhysicalModelPtr;

class OffworldPipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_supports_snapshot = 1;
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        AndroidPipe::initThreadingForTest(TestVmLock::getInstance(),
                                          mLooper.get());
        // TestLooper -> base::Looper -> C Looper
        android_registerMainLooper(reinterpret_cast<::Looper*>(
                static_cast<base::Looper*>(mLooper.get())));

        mPhysicalModel.reset(physicalModel_new());
        mAutomationController = AutomationController::createForTest(
                mPhysicalModel.get(), mLooper.get(), sendResponse);
        mVideoInjectionController = VideoInjectionController::createForTest(
                sendResponse);

        registerOffworldPipeServiceForTest(
            mAutomationController.get(), mVideoInjectionController.get());

        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "host", "off", 0, false,
                                     false, false,
                                     WINSYS_GLESBACKEND_PREFERENCE_AUTO, false));
    }

    void TearDown() override {
        {
            // To call androidSnapshot_finalize may internally call,
            // setSnapshotCallbacks.  Allow it, but don't require the call.
            ::testing::NiceMock<MockAndroidVmOperations> mock;
            auto replacementHandle =
                    replaceMock(&MockAndroidVmOperations::mock, &mock);
            androidSnapshot_finalize();
        }

        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
        // Reset initThreading to avoid using our looper.
        AndroidPipe::initThreading(TestVmLock::getInstance());
        android_registerMainLooper(nullptr);
        mAutomationController.reset();
        mVideoInjectionController.reset();
        mPhysicalModel.reset();
        mLooper.reset();
    }

    // Returns pipe pointer.
    int openAndConnectOffworld() {
        int pipe = mDevice->connect("OffworldPipe");

        pb::ConnectHandshake connect;
        connect.set_version(android::offworld::kProtocolVersion);
        writeMessage(pipe, connect);

        auto response = readMessage<pb::ConnectHandshakeResponse>(pipe);
        EXPECT_EQ(response.result(),
                  pb::ConnectHandshakeResponse::RESULT_NO_ERROR);

        return pipe;
    }

    void writeMessage(int pipe, const google::protobuf::Message& message) {
        const int size = message.ByteSize();
        std::vector<uint8_t> data(static_cast<size_t>(size));
        EXPECT_TRUE(message.SerializeToArray(data.data(), size));

        const uint32_t payloadSize = static_cast<uint32_t>(data.size());
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(pipe, &payloadSize, sizeof(uint32_t)));

        EXPECT_THAT(mDevice->write(pipe, data), IsOk());
    }

    void writeRequest(int pipe, const std::string& request) {
        pb::Request requestMessage;
        ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
                request, &requestMessage));
        writeMessage(pipe, requestMessage);
    }

    template <typename T>
    T readMessage(int pipe) {
        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(pipe, &responseSize, sizeof(uint32_t)));

        if (!responseSize) {
            return T();
        }

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

    template <typename T>
    T readMessageBlocking(int pipe) {
        const base::Looper::Duration deadline = mLooper->nowMs() + kTimeoutMs;

        // Since these unittests never expect long-running operations, run in
        // a loop with only a 1ms minimal sleep.
        while ((mDevice->poll(pipe) & PIPE_POLL_IN) == 0 &&
               mLooper->nowMs() < deadline) {
            mLooper->runOneIterationWithDeadlineMs(mLooper->nowMs() + 1);
        }

        return readMessage<T>(pipe);
    }

    void pumpLooperUntilEvent(TestEvent& event) {
        constexpr base::Looper::Duration kStep = 50;  // 50 ms.

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

    void createCheckpoint(int pipe,
                          const char* snapshotName,
                          base::Stream* stream) {
        MockAndroidVmOperations mock;
        auto replacementHandle =
                replaceMock(&MockAndroidVmOperations::mock, &mock);

        EXPECT_CALL(mock, setSnapshotCallbacks(_, _)).Times(1);
        androidSnapshot_initialize(getConsoleAgents()->vm,
                                  getConsoleAgents()->emu);
        Mock::VerifyAndClearExpectations(&mock);

        EXPECT_CALL(mock, vmStop()).Times(1);
        EXPECT_CALL(mock, snapshotSave(StrEq(snapshotName), _, _)).Times(1);
        EXPECT_CALL(mock, vmStart()).Times(1);

        ON_CALL(mock, snapshotSave(_, _, _))
                .WillByDefault(::testing::Invoke(
                        [stream](const char*, void*, LineConsumerCallback) {
                            android::snapshot::onOffworldSave(stream);
                            return true;
                        }));
        pb::Request request;
        request.mutable_snapshot()
                ->mutable_create_checkpoint()
                ->set_snapshot_name(snapshotName);
        writeMessage(pipe, request);

        pb::Response response = readMessageBlocking<pb::Response>(pipe);
        EXPECT_EQ(response.result(), pb::Response::RESULT_NO_ERROR);
    }

    void gotoCheckpoint(int pipe,
                        const char* snapshotName,
                        base::Stream* stream) {
        MockAndroidVmOperations mock;
        auto replacementHandle =
                replaceMock(&MockAndroidVmOperations::mock, &mock);

        EXPECT_CALL(mock, vmStop()).Times(1);
        EXPECT_CALL(mock, snapshotLoad(StrEq(snapshotName), _, _)).Times(1);
        EXPECT_CALL(mock, vmStart()).Times(1);

        ON_CALL(mock, snapshotLoad(_, _, _))
                .WillByDefault(::testing::Invoke(
                        [stream](const char*, void*, LineConsumerCallback) {
                            android::snapshot::onOffworldLoad(stream);
                            return true;
                        }));
        {
            static std::string kMetadata = "test metadata";

            pb::Request request;
            auto requestCheckpoint =
                    request.mutable_snapshot()->mutable_goto_checkpoint();
            requestCheckpoint->set_snapshot_name(snapshotName);
            requestCheckpoint->set_metadata(kMetadata);
            writeMessage(pipe, request);

            pb::Response response = readMessageBlocking<pb::Response>(pipe);
            EXPECT_EQ(response.result(), pb::Response::RESULT_NO_ERROR);
            EXPECT_EQ(response.snapshot().function_case(),
                      pb::SnapshotResponse::FunctionCase::kCreateCheckpoint);
            EXPECT_EQ(response.snapshot().create_checkpoint().metadata(),
                      kMetadata);
            Mock::VerifyAndClearExpectations(&mock);
        }
    }

    void advanceTimeToNs(uint64_t timeNs) {
        mLooper->setVirtualTimeNs(timeNs);
        EXPECT_EQ(mAutomationController->advanceTime(), timeNs);
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    std::unique_ptr<TestLooper> mLooper;
    PhysicalModelPtr mPhysicalModel;
    std::unique_ptr<AutomationController> mAutomationController;
    std::unique_ptr<VideoInjectionController> mVideoInjectionController;
};

TEST_F(OffworldPipeTest, Connect) {
    int pipe = mDevice->connect("OffworldPipe");

    pb::ConnectHandshake connect;
    connect.set_version(android::offworld::kProtocolVersion);

    writeMessage(pipe, connect);

    auto response = readMessage<pb::ConnectHandshakeResponse>(pipe);
    EXPECT_EQ(response.result(), pb::ConnectHandshakeResponse::RESULT_NO_ERROR);

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, ConnectSnapshotBeforeHandshake) {
    base::MemStream snapshotStream;
    {
        int pipe = mDevice->connect("OffworldPipe");
        snapshotSave(pipe, &snapshotStream);
        mDevice->close(pipe);
    }

    auto restoredPipe = snapshotLoad(&snapshotStream);

    pb::ConnectHandshake connect;
    connect.set_version(android::offworld::kProtocolVersion);

    writeMessage(restoredPipe, connect);

    auto response = readMessage<pb::ConnectHandshakeResponse>(restoredPipe);
    EXPECT_EQ(response.result(), pb::ConnectHandshakeResponse::RESULT_NO_ERROR);

    mDevice->close(restoredPipe);
}

TEST_F(OffworldPipeTest, ConnectionFailure) {
    int pipe = mDevice->connect("OffworldPipe");

    TestEvent receivedClose;
    mDevice->setWakeCallback(pipe, [&](int wakes) {
        if (wakes & PIPE_WAKE_CLOSED) {
            receivedClose.signal();
        }
    });

    pb::ConnectHandshake connect;
    connect.set_version(UINT32_MAX);

    writeMessage(pipe, connect);

    auto response = readMessage<pb::ConnectHandshakeResponse>(pipe);
    EXPECT_EQ(response.result(),
              pb::ConnectHandshakeResponse::RESULT_ERROR_VERSION_MISMATCH);

    // Check that the pipe was closed by the host.
    pumpLooperUntilEvent(receivedClose);

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, Snapshot) {
    int pipe = openAndConnectOffworld();

    MemStream stream;
    createCheckpoint(pipe, "TestSnapshot", &stream);
    gotoCheckpoint(pipe, "TestSnapshot", &stream);

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, SnapshotMultiPipe) {
    int commandPipe = openAndConnectOffworld();
    int otherPipe = openAndConnectOffworld();

    MemStream stream;
    createCheckpoint(commandPipe, "TestSnapshot", &stream);
    gotoCheckpoint(commandPipe, "TestSnapshot", &stream);

    // The other pipe should not have received anything.
    EXPECT_EQ(mDevice->poll(otherPipe), PIPE_POLL_OUT);

    mDevice->close(commandPipe);
    mDevice->close(otherPipe);
}

// TODO(jwmcglynn): Add test coverage for fork API.

TEST_F(OffworldPipeTest, AutomationListen) {
    int pipe = openAndConnectOffworld();

    writeRequest(pipe, "automation { listen {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1 "
                                  "automation { listen {} }")));

    const uint64_t kEventTime1 = 123;
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};

    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR async { async_id: 1 "
                                  " automation { event_generated {} } }")));

    writeRequest(pipe, "automation { stop_listening {} }");

    EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                EqualsProto("result: RESULT_NO_ERROR async { async_id: 1 "
                            "complete: true }"));

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, AutomationListenBlocksMultiple) {
    int pipe = openAndConnectOffworld();

    writeRequest(pipe, "automation { listen {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1 "
                                  "automation { listen {} }")));

    writeRequest(pipe, "automation { listen {} }");

    EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                Partially(EqualsProto("result: RESULT_ERROR_UNKNOWN "
                                      "error_string: \"Already listening\"")));
    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, AutomationListenPipeClose) {
    int pipe = openAndConnectOffworld();

    writeRequest(pipe, "automation { listen {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1 "
                                  "automation { listen {} }")));
    mDevice->close(pipe);

    int secondPipe = openAndConnectOffworld();
    writeRequest(secondPipe, "automation { listen {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(secondPipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1 "
                                  "automation { listen {} }")));
    mDevice->close(secondPipe);
}

TEST_F(OffworldPipeTest, AutomationReplay) {
    int pipe = openAndConnectOffworld();
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    const uint64_t kFutureTime = secondsToNs(10.0f);

    writeRequest(pipe,
                 "automation { replay { event: 'delay: 123 "
                 "physical_model { type: POSITION target_value { "
                 "data: [0.5, 10, 0] } } ' } }");

    EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1"));

    // Advance the time so that the event completes.
    mLooper->setVirtualTimeNs(kFutureTime);
    EXPECT_EQ(mAutomationController->advanceTime(), kFutureTime);

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR async { async_id: 1 "
                                  "complete: true automation { "
                                  "replay_complete {} } }")));

    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kPosition1);

    mDevice->close(pipe);
}

TEST_F(OffworldPipeTest, AutomationAsyncIdSnapshot) {
    base::MemStream snapshotStream;
    {
        int pipe = openAndConnectOffworld();

        writeRequest(pipe,
                     "automation { replay { event: 'delay: 123 "
                     "physical_model { type: POSITION target_value { "
                     "data: [0.5, 10, 0] } } ' } }");

        EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                    EqualsProto("result: RESULT_NO_ERROR pending_async_id: 1"));

        snapshotSave(pipe, &snapshotStream);
        mDevice->close(pipe);
    }

    auto restoredPipe = snapshotLoad(&snapshotStream);

    writeRequest(restoredPipe,
                 "automation { replay { event: 'delay: 123 "
                 "physical_model { type: POSITION target_value { "
                 "data: [0.5, 10, 0] } } ' } }");

    EXPECT_THAT(readMessageBlocking<pb::Response>(restoredPipe),
                EqualsProto("result: RESULT_NO_ERROR pending_async_id: 2"));

    mDevice->close(restoredPipe);
}

TEST_F(OffworldPipeTest, VideoInjection) {
    int pipe = openAndConnectOffworld();

    writeRequest(
        pipe,
        "video_injection { display_default_frame {} } ");

    EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                Partially(EqualsProto(
                    "result: RESULT_NO_ERROR pending_async_id: 1")));

    writeRequest(
        pipe,
        "video_injection { display_default_frame {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 2 "
                                  "video_injection {sequence_id: 0}")));

    writeRequest(
        pipe,
        "video_injection {sequence_id: 100 display_default_frame {} }");

    EXPECT_THAT(
            readMessageBlocking<pb::Response>(pipe),
            Partially(EqualsProto("result: RESULT_NO_ERROR pending_async_id: 3 "
                                  "video_injection {sequence_id: 100}")));

    writeRequest(
        pipe,
        "video_injection { }");

    EXPECT_THAT(readMessageBlocking<pb::Response>(pipe),
                Partially(EqualsProto("result: RESULT_ERROR_UNKNOWN "
                                      "error_string: \"Invalid request\"")));

    mDevice->close(pipe);
}
