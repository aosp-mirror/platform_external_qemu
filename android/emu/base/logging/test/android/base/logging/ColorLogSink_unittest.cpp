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
#include "android/base/logging/ColorLogSink.h"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log_sink_registry.h"

#include "aemu/base/logging/Log.h"

namespace android::base {

void initlogging() {
  static bool mInitialized = false;
  if (!mInitialized) absl::InitializeLog();
  mInitialized = true;
}


enum class Console { NO_TTY = 0, TTY = 1 };

class LogSinkTest :  public ::testing::TestWithParam<Console> {
 public:
  LogSinkTest() : mColorSink((int) GetParam()) { initlogging(); }
  void SetUp() override {
    mStr.clear();
    absl::SetMinLogLevel(static_cast<absl::LogSeverityAtLeast>(-2));
    absl::AddLogSink(&mStr, &mColorSink)
  }

  void TearDown() override {
    absl::RemoveLogSink(&mColorSink);
  }

 protected:
  std::stringstream mStr;
  ColorLogSink mColorSink;
};

TEST_P(LogSinkTest, WritesColor) {
  // Check for each log level
  for (int i = static_cast<int>(absl::LogSeverity::kInfo);
       i <= static_cast<int>(absl::LogSeverity::kFatal); ++i) {
    absl::LogSeverity severity = static_cast<absl::LogSeverity>(i);

    // Get the color string for the current severity
    auto colorString = mColorSink.color(severity);

    // Assert based on GetParam()
    if (GetParam() == Console::TTY) {
      // Color should be present
      EXPECT_FALSE(colorString.empty())
          << "Color string is empty for severity " << absl::LogSeverityName(severity)
          << " when color (TTY) is enabled.";
    } else {
      // Color should be absent
      EXPECT_TRUE(colorString.empty())
          << "Color string is not empty for severity " << absl::LogSeverityName(severity)
          << " when color (NO TTY) is disabled.";
    }
  }
}
TEST_P(LogSinkTest, LogsInfo) {
  LOG(INFO) << "Hello world!";
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr(mColorSink.color(absl::LogSeverity::kInfo)));
}

TEST_P(LogSinkTest, LogsWarning) {
  LOG(WARNING) << "Hello world!";
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr(mColorSink.color(absl::LogSeverity::kWarning)));
}

TEST_P(LogSinkTest, LogsError) {
  LOG(ERROR) << "Hello world!";
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr("Hello world!"));
  EXPECT_THAT(mStr.str(), ::testing::HasSubstr(mColorSink.color(absl::LogSeverity::kError)));
}

TEST_P(LogSinkTest, LogsFatal) {
  // We are going to crash, so we will not get to see any data written
  // to our stream... We can however observe std::cerr
  internal::setColorLogStream(&std::cerr);
  EXPECT_DEATH(LOG(FATAL) << "Hello world!", "Hello world!");
}


INSTANTIATE_TEST_SUITE_P(WithAndWithoutColor,
                         LogSinkTest,
                          ::testing::Values(Console::NO_TTY, Console::TTY),
                         [](const ::testing::TestParamInfo<Console>& info) {
                           return info.param == Console::TTY ? "TTY" : "NO_TTY";
                         });

}  // namespace android::base