// Copyright 2019 The Android Open Source Project
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

#include <chrono>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/CLog.h"
#include "aemu/base/process/Command.h"
#include "android/base/system/System.h"

#include "client/crash_report_database.h"
#include "client/settings.h"

#include <gtest/gtest.h>

using android::base::Command;
using android::base::PathUtils;
using android::base::System;
using crashpad::CrashReportDatabase;
using namespace std::chrono_literals;

const constexpr char kCrashpadDatabase[] = "emu-crash.db";

// The crashpad handler binary, as shipped with the emulator.
const constexpr char kCrashpadHandler[] = "crashpad_handler";
const std::string kCrashMe = "crash-me";

class CrashTest : public ::testing::Test {
protected:
    void SetUp() override { mCrashdatabase = InitializeCrashDatabase(); }

    std::unique_ptr<CrashReportDatabase> InitializeCrashDatabase() {
        auto crashDatabasePath = android::base::pj(System::get()->getTempDir(),
                                                   kCrashpadDatabase);
        auto handler_path = ::base::FilePath(
                PathUtils::asUnicodePath(
                        System::get()
                                ->findBundledExecutable(kCrashpadHandler)
                                .data())
                        .c_str());

        android::base::pj(System::get()->getTempDir(), kCrashpadDatabase);
        auto database_path = ::base::FilePath(
                PathUtils::asUnicodePath(crashDatabasePath.data()).c_str());
        auto crashDatabase =
                crashpad::CrashReportDatabase::Initialize(database_path);

        crashDatabase->GetSettings()->SetUploadsEnabled(false);

        return crashDatabase;
    }

    void crash() {
        std::string executable = System::get()->findBundledExecutable(kCrashMe);
        dinfo("Running %s", executable.c_str());
        auto proc = Command::create({executable}).execute();

        // Let's give the crash handler some time to write to the database.
        std::this_thread::sleep_for(1s);
    }

    std::unique_ptr<CrashReportDatabase> mCrashdatabase;
};

TEST_F(CrashTest, crash_generates_minidump) {
    std::vector<CrashReportDatabase::Report> reports;
    std::vector<CrashReportDatabase::Report> pendingReports;
    mCrashdatabase->GetCompletedReports(&reports);
    mCrashdatabase->GetPendingReports(&pendingReports);

    auto before = reports.size() + pendingReports.size();

    crash();

    std::vector<CrashReportDatabase::Report> newReports;
    std::vector<CrashReportDatabase::Report> newPendingReports;
    mCrashdatabase->GetCompletedReports(&newReports);
    mCrashdatabase->GetPendingReports(&newPendingReports);
    auto after = newReports.size() + newPendingReports.size();

    EXPECT_GT(after, before)
            << "The database should have recorded an additional crash!";
}

TEST_F(CrashTest, minidump_is_usable) {
   // TODO: This requires us to binplace all the .sym files ourselves.
}
