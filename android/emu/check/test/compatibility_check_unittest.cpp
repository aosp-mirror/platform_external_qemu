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
#include "android/emulation/compatibility_check.h"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include "absl/base/call_once.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace android {
namespace emulation {

using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::StartsWith;

// Matcher to check if a string is present in a vector of string_views.
MATCHER_P(ContainsString, expected_string, "") {
    for (const auto& str_view : arg) {
        if (str_view == expected_string) {
            return true;
        }
    }
    return false;
}

struct CaptureLogSink : public absl::LogSink {
public:
    void Send(const absl::LogEntry& entry) override {
        captured_log_ += absl::StrFormat("%s\n", entry.text_message());
    }

    void clear() { captured_log_.clear(); }

    std::string captured_log_;
};

void initlogs_once() {
    static bool mInitialized = false;
    if (!mInitialized)
        absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
    mInitialized = true;
}

static absl::once_flag initlogs;
class AvdCompatibilityCheckResultTest : public ::testing::Test {
protected:
    AvdCompatibilityCheckResultTest() { absl::call_once(initlogs, initlogs_once); }
    void SetUp() override {
        // Add the CaptureLogSink
        log_sink_ = std::make_unique<CaptureLogSink>();
        absl::AddLogSink(log_sink_.get());
        absl::SetVLogLevel("*", 2);

        oldCout = std::cout.rdbuf();
        std::cout.rdbuf(buffer.rdbuf());
    }

    void TearDown() override {
        // Remove the CaptureLogSink
        absl::RemoveLogSink(log_sink_.get());
        // Capture and restore stdout
        std::cout.rdbuf(oldCout);
        buffer.clear();
    }

    std::unique_ptr<CaptureLogSink> log_sink_;
    std::streambuf* oldCout;
    std::stringstream buffer;
};

struct AvdCompatibilityManagerTest {
    static void clear() { AvdCompatibilityManager::instance().mChecks.clear(); }
};

AvdCompatibilityCheckResult sampleOkayCheck(AvdInfo* foravd) {
    return {
            .description = "Sample Ok Verification",
            .status = AvdCompatibility::Ok,
    };
};

AvdCompatibilityCheckResult alwaysErrorBar(AvdInfo* foravd) {
    return {
            .description = "You need more bar!",
            .status = AvdCompatibility::Error,
    };
};

AvdCompatibilityCheckResult alwaysErrorFoo(AvdInfo* foravd) {
    return {
            .description = "You need more foo!",
            .status = AvdCompatibility::Error,
    };
};

REGISTER_COMPATIBILITY_CHECK(sampleOkayCheck);
REGISTER_COMPATIBILITY_CHECK(alwaysErrorBar);
REGISTER_COMPATIBILITY_CHECK(alwaysErrorFoo);

TEST_F(AvdCompatibilityCheckResultTest, auto_registration_should_work) {
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("sampleOkayCheck"));
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("alwaysErrorBar"));
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("alwaysErrorFoo"));
}

TEST_F(AvdCompatibilityCheckResultTest, auto_registration_results_print_user_messages) {
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().printResults(results);
    EXPECT_THAT(buffer.str(), testing::HasSubstr("USER_ERROR   | You need more bar!  You need more foo!"));
}

TEST_F(AvdCompatibilityCheckResultTest, actually_calls_registered_tests) {
    bool wascalled = false;
    AvdCompatibilityManager::instance().registerCheck(
            [&wascalled](AvdInfo* info) {
                wascalled = true;
                return AvdCompatibilityCheckResult{
                        "This check always fails with an error!",
                        AvdCompatibility::Error};
            },
            "dynamic_test");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    EXPECT_THAT(wascalled, testing::Eq(true));
}

}  // namespace emulation
}  // namespace android
