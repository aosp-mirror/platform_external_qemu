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
#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestMemoryOutputStream.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/physics/PhysicalModel.h"

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
        mController = AutomationController::createForTest(mPhysicalModel.get(),
                                                          mLooper.get());
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

    TestSystem mTestSystem;
    std::unique_ptr<TestLooper> mLooper;
    PhysicalModelPtr mPhysicalModel;
    std::unique_ptr<AutomationController> mController;
};

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

TEST_F(AutomationControllerTest, SimpleRecordPlay) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopRecording(), IsOk());

    EXPECT_THAT(mController->startPlayback("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopPlayback(), IsOk());
}

TEST_F(AutomationControllerTest, DuplicateRecord) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->startRecording("otherFile.txt"),
                IsErr(StartError::AlreadyStarted));
}

TEST_F(AutomationControllerTest, DuplicatePlay) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());
    EXPECT_THAT(mController->stopRecording(), IsOk());

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
            new StdioStream(fopen(path.c_str(), "wb"), StdioStream::kOwner));
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

    advanceTimeToNs(kEventTime1);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition1,
                                    PHYSICAL_INTERPOLATION_SMOOTH);

    advanceTimeToNs(kEventTime2);
    physicalModel_setTargetPosition(mPhysicalModel.get(), kPosition2,
                                    PHYSICAL_INTERPOLATION_STEP);

    advanceTimeToNs(kEventTime3);
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

    EXPECT_THAT(mController->stopPlayback(), IsOk());
}

TEST_F(AutomationControllerTest, DestructWhileRecording) {
    EXPECT_THAT(mController->startRecording("testRecording.txt"), IsOk());

    const uint64_t kEventTime1 = secondsToNs(0.5);
    const vec3 kPosition1 = {0.5f, 10.0f, 0.0f};
    advanceTimeToNs(kEventTime1);
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
