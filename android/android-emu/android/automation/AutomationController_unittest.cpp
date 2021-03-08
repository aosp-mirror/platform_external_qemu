// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/automation/AutomationController.h"

#include "android/automation/AutomationEventSink.h"
#include "android/base/files/StdioStream.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/testing/ProtobufMatchers.h"
#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestMemoryOutputStream.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "offworld.pb.h"
#include "android/physics/PhysicalModel.h"

#include <gmock/gmock.h>
#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

using namespace android::automation;
using android::base::IsErr;
using android::base::IsOk;
using android::base::System;
using android::base::TestLooper;
using android::base::TestMemoryOutputStream;
using android::base::TestSystem;
using android::base::TestTempDir;

using android::EqualsProto;
using android::proto::Partially;
using testing::_;
using testing::Eq;

constexpr uint64_t secondsToNs(float seconds) {
    return static_cast<uint64_t>(seconds * 1000000000.0);
}

struct PhysicalModelDeleter {
    void operator()(PhysicalModel* physicalModel) {
        physicalModel_free(physicalModel);
    }
};

typedef std::unique_ptr<PhysicalModel, PhysicalModelDeleter> PhysicalModelPtr;

void PrintTo(const vec3& value, std::ostream* os) {
    *os << "vec3(" << value.x << ", " << value.y << ", " << value.z << ")";
}

TEST(AutomationController, CreateAndDestroy) {
    TestLooper looper;
    PhysicalModelPtr model(physicalModel_new());
    std::unique_ptr<AutomationController> controller =
            AutomationController::createForTest(model.get(), &looper);
    EXPECT_TRUE(controller);
}

class AutomationControllerTest : public testing::Test {
protected:
    AutomationControllerTest() : mTestSystem("/", System::kProgramBitness) {
        TestTempDir* testDir = mTestSystem.getTempRoot();
        EXPECT_TRUE(testDir->makeSubDir("home"));
        mTestSystem.setHomeDirectory(testDir->makeSubPath("home"));
    }

    void SetUp() override {
        mLooper.reset(new TestLooper());
        mPhysicalModel.reset(physicalModel_new());
        mController = AutomationController::createForTest(
                mPhysicalModel.get(), mLooper.get(),
                [this](android::AsyncMessagePipeHandle pipe,
                       const ::offworld::Response& response) {
                    offworldSendResponse(pipe, response);
                });
    }

    void TearDown() override {
        mController.reset();
        mPhysicalModel.reset();
        mLooper.reset();
    }

    void advanceTimeToNs(uint64_t timeNs) {
        mLooper->setVirtualTimeNs(timeNs);
        EXPECT_EQ(mController->advanceTime(), timeNs);
    }

    void recordOffworldEvents(std::vector<std::string>* events,
                              android::AsyncMessagePipeHandle pipe,
                              uint32_t asyncId) {
        std::string proto = std::string("result: RESULT_NO_ERROR\n") +
                            "async {\n"
                            "    async_id: " + std::to_string(asyncId) + "\n"
                            "    automation { event_generated {} }\n"
                            "}";

        ON_CALL(*this,
                offworldSendResponse(Eq(pipe), Partially(EqualsProto(proto))))
                .WillByDefault(::testing::Invoke(
                        [events](android::AsyncMessagePipeHandle handle,
                                 const ::offworld::Response& response) {
                            events->push_back(response.async()
                                                      .automation()
                                                      .event_generated()
                                                      .event());
                        }));
    }

    static void onStopCallback() { sCallbackInt++; }

    MOCK_METHOD2(offworldSendResponse,
                 void(android::AsyncMessagePipeHandle,
                      const ::offworld::Response&));

    TestSystem mTestSystem;
    std::unique_ptr<TestLooper> mLooper;
    PhysicalModelPtr mPhysicalModel;
    std::unique_ptr<AutomationController> mController;
    static int sCallbackInt;
};

int AutomationControllerTest::sCallbackInt;

TEST_F(AutomationControllerTest, InvalidInput) {
    EXPECT_THAT(mController->startRecording(nullptr),
                IsErr(StartError::InvalidFilename));
    EXPECT_THAT(mController->startRecording(""),
                IsErr(StartError::InvalidFilename));
    EXPECT_THAT(mController->startRecording("\0"),
                IsErr(StartError::FileOpenError));

    EXPECT_THAT(mController->startPlayback(nullptr),
                IsErr(StartError::InvalidFilename));
    EXPECT_THAT(mController->startPlayback(""),
                IsErr(StartError::InvalidFilename));
    EXPECT_THAT(mController->startPlayback("file-does-not-exist.txt"),
                IsErr(StartError::FileOpenError));
}

TEST_F(AutomationControllerTest, SimpleRecordPlayEmpty) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopRecording(), IsOk());

    // Recordings now always contain a simple final empty event.
    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());

    // Advancing time because of simple end of file event.
    advanceTimeToNs(secondsToNs(1));
    EXPECT_THAT(mController->stopPlayback(), IsErr(StopError::NotStarted));
}

TEST_F(AutomationControllerTest, SimpleRecordPlay) {
    // Non empty recording.
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    EXPECT_THAT(mController->stopRecording(), IsOk());

    // Playback stops before all commands are excecuted.
    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopPlayback(), IsOk());
}

TEST_F(AutomationControllerTest, DuplicateRecord) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->startRecording("otherFile.txt"),
                IsErr(StartError::AlreadyStarted));
}

TEST_F(AutomationControllerTest, DuplicatePlay) {
    // Non empty recording.
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    EXPECT_THAT(mController->stopRecording(), IsOk());

    // Start second recording before first ends.
    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->startPlayback("testRecording.txt"),
                IsErr(StartError::AlreadyStarted));
}

TEST_F(AutomationControllerTest, PlaybackFileCorrupt) {
    using android::base::StdioStream;

    const char* kFilename = "corruptFile.txt";
    const std::string path = android::base::PathUtils::join(
            System::get()->getHomeDirectory(), kFilename);

    std::unique_ptr<StdioStream> stream(
            new StdioStream(android_fopen(path.c_str(), "wb"), StdioStream::kOwner));
    ASSERT_TRUE(stream->get());
    stream->putBe32(0);
    stream->putString("Test string please ignore");
    stream.reset();

    EXPECT_THAT(mController->startPlayback(kFilename),
                IsErr(StartError::PlaybackFileCorrupt));
}

TEST_F(AutomationControllerTest, RecordPlayEvents) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const vec3 kVecZero = {0.0f, 0.0f, 0.0f};

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    const uint64_t kEventTime2 = secondsToNs(0.6);
    const vec3 kPosition2 = {0.0f, -5.0f, -1.0f};
    const uint64_t kEventTime3 = secondsToNs(3.0);
    const vec3 kVelocity = {1.0f, 0.0f, 0.0f};
    const uint64_t kPlaybackStartTime = secondsToNs(100.0);

    // Need a 4th state so recording doesn't stop by itself.
    const uint64_t kEventTime4 = secondsToNs(120.0);

    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition2,
                                    PHYSICAL_INTERPOLATION_STEP);

    advanceTimeToNs(kEventTime3);
    physicalModel_setTargetVelocity(mPhysicalModel.get(), kVelocity,
                                    PHYSICAL_INTERPOLATION_STEP);

    advanceTimeToNs(kEventTime4);
    physicalModel_setTargetVelocity(mPhysicalModel.get(), kVelocity,
                                    PHYSICAL_INTERPOLATION_STEP);
    EXPECT_THAT(mController->stopRecording(), IsOk());

    advanceTimeToNs(kPlaybackStartTime);
    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());

    // Verify that the initial state was properly reset.
    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero);
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero);

    // Now go through the events and validate.
    advanceTimeToNs(kPlaybackStartTime + kEventTime1);

    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero)
            << "Value should not be set yet, no time to interpolate";

    advanceTimeToNs(kPlaybackStartTime + kEventTime2);
    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kPosition2)
            << "Current value should be set by step";

    advanceTimeToNs(kPlaybackStartTime + kEventTime3);
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kVelocity);

    advanceTimeToNs(kPlaybackStartTime + kEventTime3 + secondsToNs(10.0));
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVelocity);

    EXPECT_THAT(mController->stopPlayback(), IsOk());

    // Velocity was different from zero before stopping playback.
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kVecZero);
}

TEST_F(AutomationControllerTest, PlaybackStops) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const uint64_t kEventTime2 = secondsToNs(1);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);
    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->stopRecording(), IsOk());

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());

    // Check Playback stops after last event.
    advanceTimeToNs(kEventTime2 + secondsToNs(1));
    EXPECT_THAT(mController->stopPlayback(), IsErr(StopError::NotStarted));
}

TEST_F(AutomationControllerTest, PlaybackWithCallback) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopRecording(), IsOk());

    sCallbackInt = 0;

    EXPECT_THAT(mController->startPlaybackWithCallback(
                        "testRecording.txt",
                        &AutomationControllerTest::onStopCallback),
                IsOk());

    // Advancing time because of simple end of file event.
    advanceTimeToNs(secondsToNs(1));
    EXPECT_EQ(sCallbackInt, 1);
}

TEST_F(AutomationControllerTest, DestructWhileRecording) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const uint64_t kEventTime2 = secondsToNs(1);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);
    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    TearDown();
    SetUp();

    EXPECT_THAT(mController->stopRecording(), IsErr(StopError::NotStarted));

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());

    // Now go through the events and validate.
    advanceTimeToNs(kEventTime1);
    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kPosition1);

    EXPECT_THAT(mController->stopPlayback(), IsOk());

    const vec3 kVecZero = {0.0f, 0.0f, 0.0f};
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero);
}

TEST_F(AutomationControllerTest, DestructWhilePlaying) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->stopRecording(), IsOk());

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());
}

TEST_F(AutomationControllerTest, PlayWhileRecording) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->startPlayback("testRecording.txt"),
                IsErr(StartError::FileOpenError));
}

TEST_F(AutomationControllerTest, RecordWhilePlaying) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->stopRecording(), IsOk());

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->startRecording("testRecording.txt"),
                IsErr(StartError::FileOpenError));
}

TEST_F(AutomationControllerTest, Listen) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->listen(pipe, 456), IsOk());

    EXPECT_CALL(*this, offworldSendResponse(
                               Eq(pipe), EqualsProto("result: RESULT_NO_ERROR\n"
                                                     "async {\n"
                                                     "    async_id: 456\n"
                                                     "    complete: true\n"
                                                     "}")))
            .Times(1);

    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(AutomationControllerTest, ListenInitialState) {
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    const vec3 kPosition2 = {1.5f, -50.0f, -1.0f};

    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    const std::string initialState = mController->listen(pipe, 456).unwrap();

    SCOPED_TRACE(testing::Message() << "initialState=" << initialState);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition2,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->replayInitialState(initialState), IsOk());

    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kPosition1);
}

TEST_F(AutomationControllerTest, ListenEvents) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->listen(pipe, 456), IsOk());

    EXPECT_CALL(*this,
                offworldSendResponse(Eq(pipe),
                                     EqualsProto("result: RESULT_NO_ERROR\n"
                                                 "async {\n"
                                                 "    async_id: 456\n"
                                                 "    automation {\n"
                                                 "        event_generated {\n"
                                                 "            event: \"delay: "
                                                 "500000000 physical_model { "
                                                 "type: POSITION current_value "
                                                 "{ data: [0.5, 10, 0] } } \"\n"
                                                 "        }\n"
                                                 "    }\n"
                                                 "}")))
            .Times(1);

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse(
                               Eq(pipe), EqualsProto("result: RESULT_NO_ERROR\n"
                                                     "async {\n"
                                                     "    async_id: 456\n"
                                                     "    complete: true\n"
                                                     "}\n")))
            .Times(1);

    mController->stopListening();
}

TEST_F(AutomationControllerTest, ListenNoEventsAfterStop) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->listen(pipe, 456), IsOk());

    EXPECT_CALL(*this, offworldSendResponse(
                               Eq(pipe), EqualsProto("result: RESULT_NO_ERROR\n"
                                                     "async {\n"
                                                     "    async_id: 456\n"
                                                     "    complete: true\n"
                                                     "}\n")))
            .Times(1);

    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(AutomationControllerTest, InitialState) {
    EXPECT_THAT(mController->replayInitialState(""), IsOk());
}

TEST_F(AutomationControllerTest, BadInitialState) {
    EXPECT_THAT(mController->replayInitialState("not a protobuf"),
                IsErr(ReplayError::ParseError));
    EXPECT_THAT(mController->replayInitialState(std::string(5, '\0')),
                IsErr(ReplayError::ParseError));
}

TEST_F(AutomationControllerTest, BadReplay) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->replayEvent(pipe, "", 123),
                IsErr(ReplayError::ParseError));
    EXPECT_THAT(mController->replayEvent(pipe, "\n", 123),
                IsErr(ReplayError::ParseError));
    EXPECT_THAT(
            mController->replayEvent(pipe,
                                     "physical_model { type: POSITION "
                                     "target_value { data: [0.5, 10, 0] } }",
                                     123),
            IsErr(ReplayError::ParseError));
}

TEST_F(AutomationControllerTest, TimeOnlyReplay) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->replayEvent(pipe, "delay: 500000000", 123),
                IsOk());
    EXPECT_CALL(*this,
                offworldSendResponse(
                        Eq(pipe),
                        Partially(EqualsProto(
                                "async { async_id: 123 complete: true }"))))
            .Times(1);
}

TEST_F(AutomationControllerTest, Replay) {
    const vec3 kVecZero = {0.0f, 0.0f, 0.0f};

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    const uint64_t kEventTime2 = secondsToNs(0.6);
    const vec3 kPosition2 = {0.0f, -5.0f, -1.0f};
    const uint64_t kEventTime3 = secondsToNs(3.0);
    const vec3 kVelocity = {1.0f, 0.0f, 0.0f};
    const uint64_t kPlaybackStartTime = secondsToNs(100.0);

    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    const std::string initialState = mController->listen(pipe, 456).unwrap();
    SCOPED_TRACE(testing::Message() << "initialState=" << initialState);

    std::vector<std::string> events;
    recordOffworldEvents(&events, pipe, 456);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(3);

    //
    // Recording
    //

    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition2,
                                    PHYSICAL_INTERPOLATION_STEP);

    advanceTimeToNs(kEventTime3);
    physicalModel_setTargetVelocity(mPhysicalModel.get(), kVelocity,
                                    PHYSICAL_INTERPOLATION_STEP);
    testing::Mock::VerifyAndClear(this);

    SCOPED_TRACE(testing::Message()
                 << "events=" << testing::PrintToString(events));

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    //
    // Playback
    //

    advanceTimeToNs(kPlaybackStartTime);
    EXPECT_THAT(mController->replayInitialState(initialState), IsOk());

    EXPECT_THAT(mController->replayEvent(
                        pipe, android::base::join(events, "\n"), 789),
                IsOk());

    EXPECT_CALL(*this, offworldSendResponse(
                               Eq(pipe), Partially(EqualsProto(
                                                 "result: RESULT_NO_ERROR\n"
                                                 "async {\n"
                                                 "    async_id: 789\n"
                                                 "    complete: true\n"
                                                 "    automation {\n"
                                                 "        replay_complete {}\n"
                                                 "    }\n"
                                                 "}\n"))))
            .Times(1);

    // Verify that the initial state was properly reset.
    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero);
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kVecZero);

    // Now go through the events and validate.
    advanceTimeToNs(kPlaybackStartTime + kEventTime1);

    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVecZero)
            << "Value should not be set yet, no time to interpolate";

    advanceTimeToNs(kPlaybackStartTime + kEventTime2);
    EXPECT_EQ(physicalModel_getParameterPosition(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kPosition2)
            << "Current value should be set by step";

    advanceTimeToNs(kPlaybackStartTime + kEventTime3);
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_TARGET),
              kVelocity);

    advanceTimeToNs(kPlaybackStartTime + kEventTime3 + secondsToNs(10.0));
    EXPECT_EQ(physicalModel_getParameterVelocity(mPhysicalModel.get(),
                                                 PARAMETER_VALUE_TYPE_CURRENT),
              kVelocity);

    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(AutomationControllerTest, ReplayAppend) {
    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    const uint64_t kEventTime2 = secondsToNs(0.6);
    const vec3 kPosition2 = {0.0f, -5.0f, -1.0f};
    const uint64_t kEventTime3 = secondsToNs(3.0);
    const vec3 kVelocity = {1.0f, 0.0f, 0.0f};
    const uint64_t kPlaybackStartTime = secondsToNs(100.0);

    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    const std::string initialState = mController->listen(pipe, 456).unwrap();
    SCOPED_TRACE(testing::Message() << "initialState=" << initialState);

    std::vector<std::string> events;
    recordOffworldEvents(&events, pipe, 456);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(3);

    //
    // Recording
    //

    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition2,
                                    PHYSICAL_INTERPOLATION_STEP);

    advanceTimeToNs(kEventTime3);
    physicalModel_setTargetVelocity(mPhysicalModel.get(), kVelocity,
                                    PHYSICAL_INTERPOLATION_STEP);
    testing::Mock::VerifyAndClear(this);

    SCOPED_TRACE(testing::Message()
                 << "events=" << testing::PrintToString(events));

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    //
    // Playback
    //

    ASSERT_EQ(events.size(), 3);
    advanceTimeToNs(kPlaybackStartTime);
    EXPECT_THAT(mController->replayInitialState(initialState), IsOk());

    EXPECT_THAT(mController->replayEvent(pipe, events[0], 790), IsOk());
    EXPECT_THAT(mController->replayEvent(pipe, events[1], 791), IsOk());
    EXPECT_THAT(mController->replayEvent(pipe, events[2], 792), IsOk());

    // Now go through the events and validate.
    EXPECT_CALL(*this,
                offworldSendResponse(
                        Eq(pipe),
                        Partially(EqualsProto(
                                "async { async_id: 790 complete: true }"))))
            .Times(1);
    advanceTimeToNs(kPlaybackStartTime + kEventTime1);
    testing::Mock::VerifyAndClear(this);

    EXPECT_CALL(*this,
                offworldSendResponse(
                        Eq(pipe),
                        Partially(EqualsProto(
                                "async { async_id: 791 complete: true }"))))
            .Times(1);
    advanceTimeToNs(kPlaybackStartTime + kEventTime2);
    testing::Mock::VerifyAndClear(this);

    EXPECT_CALL(*this,
                offworldSendResponse(
                        Eq(pipe),
                        Partially(EqualsProto(
                                "async { async_id: 792 complete: true }"))))
            .Times(1);
    advanceTimeToNs(kPlaybackStartTime + kEventTime3);
    testing::Mock::VerifyAndClear(this);
}

TEST_F(AutomationControllerTest, ReplayBlocksPlayback) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->listen(pipe, 456), IsOk());
    std::vector<std::string> events;
    recordOffworldEvents(&events, pipe, 456);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);

    //
    // Recording
    //

    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->stopRecording(), IsOk());
    testing::Mock::VerifyAndClear(this);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    //
    // Playback
    //

    ASSERT_EQ(events.size(), 1);
    EXPECT_THAT(mController->replayEvent(pipe, events[0], 790), IsOk());

    EXPECT_THAT(mController->startPlayback("testRecording.txt"),
                IsErr(StartError::AlreadyStarted));

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
}

TEST_F(AutomationControllerTest, PlaybackBlocksReplay) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mController->listen(pipe, 456), IsOk());
    std::vector<std::string> events;
    recordOffworldEvents(&events, pipe, 456);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);

    //
    // Recording
    //

    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_STEP);

    EXPECT_THAT(mController->stopRecording(), IsOk());
    testing::Mock::VerifyAndClear(this);

    EXPECT_CALL(*this, offworldSendResponse(Eq(pipe), _)).Times(1);
    mController->stopListening();
    testing::Mock::VerifyAndClearExpectations(this);

    //
    // Playback
    //

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());

    ASSERT_EQ(events.size(), 1);
    EXPECT_THAT(mController->replayEvent(pipe, events[0], 790),
                IsErr(ReplayError::PlaybackInProgress));
}
