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
#include "offworld.pb.h"

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

TEST_F(VideoInjectionControllerTest, TryGetNextRequestContext) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    offworld::VideoInjectionRequest request;
    request.set_sequence_id(100);
    request.mutable_display_default_frame();

    mController->handleRequest(pipe, request, 1);
    mController->handleRequest(pipe, request, 2);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext1 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext1->request,
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext2 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext2->request,
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_FALSE(mController->getNextRequestContext());
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_FALSE(mController->getNextRequestContext());
    testing::Mock::VerifyAndClearExpectations(this);

    mController->handleRequest(pipe, request, 3);
    mController->handleRequest(pipe, request, 4);
    mController->handleRequest(pipe, request, 5);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext3 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext3->request,
                EqualsProto("sequence_id: 100 display_default_frame {}"));

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext4 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext4->request,
                EqualsProto("sequence_id: 100 display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext5 = mController->getNextRequestContext();
    EXPECT_THAT(
        requestContext5->request,
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

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext1 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext1->request,
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    mController->pipeClosed(pipe1);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext2 = mController->getNextRequestContext();
    EXPECT_THAT(requestContext2->request,
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_FALSE(mController->getNextRequestContext());
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
    EXPECT_FALSE(mController->getNextRequestContext());

    // Reset when a request is pending.
    mController->handleRequest(pipe, request, 3);
    mController->handleRequest(pipe, request, 4);
    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    android::base::Optional<::android::videoinjection::RequestContext>
            requestContext = mController->getNextRequestContext();
    EXPECT_THAT(requestContext->request,
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);
    EXPECT_CALL(*this,
                offworldSendResponse(
                        Eq(pipe), Partially(EqualsProto("result: RESULT_ERROR_UNKNOWN "
                                          "error_string: \"Internal error: Try to reset Video Injection Controller while there are pending requests.\""))))
            .Times(1);
    mController->reset();

    EXPECT_FALSE(mController->getNextRequestContext());
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(VideoInjectionControllerTest, sendFollowUpAsyncResponse) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    offworld::VideoInjectionRequest loadRequest;
    loadRequest.set_sequence_id(100);
    loadRequest.mutable_display_default_frame();

    offworld::VideoInjectionRequest playRequest;
    playRequest.set_sequence_id(200);
    playRequest.mutable_play();

    mController->handleRequest(pipe, loadRequest, 1);
    mController->handleRequest(pipe, playRequest, 2);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    mController->getNextRequestContext();
    mController->getNextRequestContext();

    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "result: RESULT_NO_ERROR async { async_id: 1 complete: true }"))))
            .Times(1);
    std::string errorDetails = "";
    mController->sendFollowUpAsyncResponse(1, android::base::Ok(), true, errorDetails);

    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "result: RESULT_ERROR_UNKNOWN async { async_id: 2}"))))
            .Times(1);

    mController->sendFollowUpAsyncResponse(2, android::base::Err(VideoInjectionError::InvalidRequest), false, errorDetails);

    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);

    mController->sendFollowUpAsyncResponse(3, android::base::Err(VideoInjectionError::InvalidRequest), false, errorDetails);

    testing::Mock::VerifyAndClearExpectations(this);

}
