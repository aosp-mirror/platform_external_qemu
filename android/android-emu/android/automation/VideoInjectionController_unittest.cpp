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

#include "android/automation/VideoInjectionController.h"

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

using namespace android::automation;
using android::base::IsErr;
using android::base::IsOk;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

using android::EqualsProto;
using android::proto::Partially;
using testing::_;
using testing::Eq;

class VideoInjectionControllerTest : public testing::Test {
protected:
    VideoInjectionControllerTest() : mTestSystem("/", System::kProgramBitness) {
        TestTempDir* testDir = mTestSystem.getTempRoot();
        EXPECT_TRUE(testDir->makeSubDir("home"));
        mTestSystem.setHomeDirectory(testDir->makeSubPath("home"));
    }

    void SetUp() override {
        mManager = VideoInjectionController::create(
                [this](android::AsyncMessagePipeHandle pipe,
                       const ::offworld::Response& response) {
                    offworldSendResponse(pipe, response);
                });
    }

    void TearDown() override {
        mManager.reset();
    }

    MOCK_METHOD2(offworldSendResponse,
                 void(android::AsyncMessagePipeHandle,
                      const ::offworld::Response&));

    TestSystem mTestSystem;
    std::unique_ptr<VideoInjectionController> mManager;
};

TEST_F(VideoInjectionControllerTest, AddCommand) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    EXPECT_THAT(mManager->addCommand(pipe, "", 123),
                IsErr(VideoInjectionError::InvalidCommand));

    EXPECT_THAT(mManager->addCommand(pipe, "\n", 123),
                IsErr(VideoInjectionError::InvalidCommand));

    EXPECT_THAT(mManager->addCommand(pipe, "display_default_frame { }", 123),
                IsOk());
}

TEST_F(VideoInjectionControllerTest, GetNextCommand) {
    android::AsyncMessagePipeHandle pipe;
    pipe.id = 123;

    mManager->addCommand(pipe, "display_default_frame { }", 1);
    mManager->addCommand(pipe, "display_default_frame { }", 2);
    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_THAT(*(mManager->getNextCommand(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "async { async_id: 1 complete: true }"))))
            .Times(1);
    EXPECT_THAT(*(mManager->getNextCommand(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "async { async_id: 2 complete: true }"))))
            .Times(1);
    EXPECT_EQ(mManager->getNextCommand(android::base::Ok()), nullptr);
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this, offworldSendResponse).Times(0);
    EXPECT_EQ(mManager->getNextCommand(android::base::Ok()), nullptr);
    testing::Mock::VerifyAndClearExpectations(this);

    mManager->addCommand(pipe, "display_default_frame { }", 3);
    mManager->addCommand(pipe, "display_default_frame { }", 4);
    mManager->addCommand(pipe, "display_default_frame { }", 5);
    EXPECT_THAT(*(mManager->getNextCommand(android::base::Ok())),
                EqualsProto("display_default_frame {}"));

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "async { async_id: 3 complete: true }"))))
            .Times(1);
    EXPECT_THAT(*(mManager->getNextCommand(android::base::Ok())),
                EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);

    EXPECT_CALL(*this,
        offworldSendResponse(
                Eq(pipe),
                Partially(EqualsProto(
                        "result: RESULT_ERROR_UNKNOWN"))))
            .Times(1);
    EXPECT_THAT(
        *(mManager->getNextCommand(
            android::base::Err(VideoInjectionError::InternalError))),
        EqualsProto("display_default_frame {}"));
    testing::Mock::VerifyAndClearExpectations(this);
}
