// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/videoinjection/VideoInjectionController.h"

#include "android/base/files/StdioStream.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/testing/ProtobufMatchers.h"
#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/offworld/proto/offworld.pb.h"

#include <gmock/gmock.h>
#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

using namespace android::videoinjection;
using android::base::IsErr;
using android::base::IsOk;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

using android::EqualsProto;
using android::proto::Partially;
using testing::_;
using testing::Eq;

TEST(VideoInjectionController, CreateAndDestroy) {
    std::unique_ptr<VideoInjectionController> controller =
            VideoInjectionController::createForTest();
    EXPECT_TRUE(controller);
}

class VideoInjectionControllerTest : public testing::Test {
protected:
    VideoInjectionControllerTest() : mTestSystem("/", System::kProgramBitness) {
        TestTempDir* testDir = mTestSystem.getTempRoot();
        EXPECT_TRUE(testDir->makeSubDir("home"));
        mTestSystem.setHomeDirectory(testDir->makeSubPath("home"));
    }

    void SetUp() override {
        mController = VideoInjectionController::createForTest(
                [this](android::AsyncMessagePipeHandle pipe,
                       const ::offworld::Response& response) {
                    offworldSendResponse(pipe, response);
                });
    }

    void TearDown() override {
        mController.reset();
    }

    MOCK_METHOD2(offworldSendResponse,
                 void(android::AsyncMessagePipeHandle,
                      const ::offworld::Response&));

    TestSystem mTestSystem;
    std::unique_ptr<VideoInjectionController> mController;
};

TEST_F(VideoInjectionControllerTest, handleRequest) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    offworld::VideoInjectionRequest emptyRequest;
    EXPECT_THAT(mController->handleRequest(pipe, emptyRequest, 123),
                IsErr(VideoInjectionError::InvalidRequest));

    offworld::VideoInjectionRequest request;
    request.mutable_display_default_frame();
    EXPECT_THAT(mController->handleRequest(pipe, request, 123),
                IsOk());
}

TEST_F(VideoInjectionControllerTest, TryGetNextRequest) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    offworld::VideoInjectionRequest request;
    request.set_sequence_id(100);
    request.mutable_display_default_frame();

    mController->handleRequest(pipe, request, 1);
    mController->handleRequest(pipe, request, 2);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
                offworldSendResponse(
                    Eq(pipe), Partially(EqualsProto(
                                  "async { async_id: 1 complete: true "
                                  "video_injection {sequence_id: 100}}"))))
        .Times(1);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
                offworldSendResponse(
                    Eq(pipe), Partially(EqualsProto(
                                  "async { async_id: 2 complete: true "
                                  "video_injection {sequence_id: 100}}"))))
        .Times(1);
    EXPECT_FALSE(mController->getNextRequest(android::base::Ok()));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_FALSE(mController->getNextRequest(android::base::Ok()));
    testing::Mock::VerifyAndClearExpectations(this);

    mController->handleRequest(pipe, request, 3);
    mController->handleRequest(pipe, request, 4);
    mController->handleRequest(pipe, request, 5);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("sequence_id: 100 display_default_frame {}"));

    EXPECT_CALL(*this,
                offworldSendResponse(
                    Eq(pipe), Partially(EqualsProto(
                                  "async { async_id: 3 complete: true "
                                  "video_injection {sequence_id: 100}}"))))
        .Times(1);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "result: RESULT_ERROR_UNKNOWN"))))
            .Times(1);
    EXPECT_THAT(
        *(mController->getNextRequest(
            android::base::Err(VideoInjectionError::InternalError))),
        EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(VideoInjectionControllerTest, PipeClosed) {
    android::AsyncMessagePipeHandle pipe1;
    pipe1.id = 123;

    android::AsyncMessagePipeHandle pipe2;
    pipe2.id = 456;

    offworld::VideoInjectionRequest request;
    request.mutable_display_default_frame();

    mController->handleRequest(pipe1, request, 1);
    mController->handleRequest(pipe2, request, 2);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    mController->pipeClosed(pipe1);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);


    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe2),
                Partially(EqualsProto(
                        "async { async_id: 2 complete: true }"))))
            .Times(1);
    EXPECT_FALSE(mController->getNextRequest(android::base::Ok()));
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(VideoInjectionControllerTest, Reset) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    offworld::VideoInjectionRequest request;
    request.mutable_display_default_frame();

    mController->handleRequest(pipe, request, 1);
    mController->handleRequest(pipe, request, 2);

    // Reset when no request is pending.
    mController->reset();
    EXPECT_FALSE(mController->getNextRequest(android::base::Ok()));

    // Reset when a request is pending.
    mController->handleRequest(pipe, request, 3);
    mController->handleRequest(pipe, request, 4);
    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_THAT(*(mController->getNextRequest(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);
    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "result: RESULT_ERROR_UNKNOWN "
                        "error_string: \"Internal error\""))))
            .Times(1);
    mController->reset();

    EXPECT_FALSE(mController->getNextRequest(android::base::Ok()));
    testing::Mock::VerifyAndClearExpectations(this);
}
