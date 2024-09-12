// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/logging/StudioLogSink.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/internal/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "android/base/logging/StudioMessage.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

namespace android::base {

void initlogging() {
    static bool mInitialized = false;
    if (!mInitialized)
        absl::InitializeLog();
    mInitialized = true;
}
class LogSinkTest : public ::testing::Test {
public:
    LogSinkTest() { initlogging(); }
    void SetUp() override {
        mStr.clear();
        studio_sink()->SetOutputStream(&mStr);
    }

    void TearDown() override {
        studio_sink()->SetOutputStream(&std::cout);
    }

protected:
    std::stringstream mStr;
};

TEST_F(LogSinkTest, LogsInfo) {
    USER_MESSAGE(INFO) << "Hello world!";
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("USER_INFO"));
}

TEST_F(LogSinkTest, LogsWarning) {
    USER_MESSAGE(WARNING) << "Hello world!";
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("USER_WARNING"));
}

TEST_F(LogSinkTest, LogsError) {
    USER_MESSAGE(ERROR) << "Hello world!";
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
    EXPECT_THAT(mStr.str(), ::testing::HasSubstr("USER_ERROR"));
}

TEST_F(LogSinkTest, LogsFatal) {
    // We are going to crash, so we will not get to see any data written
    // to our stream... We can however observe std::err
    studio_sink()->SetOutputStream(&std::cerr);
    EXPECT_DEATH(LOG(FATAL) << "Hello world!", "Hello world!\n");
}
}  // namespace android::base