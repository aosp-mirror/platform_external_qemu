// Copyright 2015 The Android Open Source Project
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
#include <fstream>  // for string
#include <map>      // for map
#include <memory>   // for unique_ptr
#include <string>   // for basic_string, operator<
#include <vector>   // for vector<>::iterator

#include "aemu/base/Compiler.h"                  // for DISALLOW_COPY_AND_ASSIGN
#include "aemu/base/files/PathUtils.h"           // for pj, PathUtils
#include "aemu/base/logging/CLog.h"              // for dinfo, dwarning
#include "aemu/base/memory/LazyInstance.h"       // for LazyInstance, LAZY_IN...
#include "android/base/system/System.h"          // for System
#include "android/crashreport/CrashConsent.h"    // for CrashReportDatabase
#include "android/crashreport/CrashReporter.h"   // for CrashReporter
#include "android/crashreport/HangDetector.h"    // for HangDetector
#include "android/version.h"                     // for EMULATOR_FULL_VERSION...
#include "client/crash_report_database.h"        // for CrashReportDatabase::...
#include "client/crashpad_client.h"              // for CrashpadClient
#include "client/settings.h"                     // for Settings
#include "mini_chromium/base/files/file_path.h"  // for FilePath
#include "util/misc/uuid.h"                      // for UUID

#ifdef _WIN32
#include <io.h>
#endif

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::System;

namespace android {
namespace crashreport {

#ifdef NDEBUG
constexpr char CrashURL[] = "https://clients2.google.com/cr/report";
#else
constexpr char CrashURL[] = "https://clients2.google.com/cr/staging_report";
#endif

const constexpr char kCrashpadDatabase[] = "emu-crash.db";

// The crashpad handler binary, as shipped with the emulator.
const constexpr char kCrashpadHandler[] = "crashpad_handler";

class CrashSystem {
public:
    CrashSystem()
        : mDatabasePath(android::base::pj(System::get()->getTempDir(),
                                          kCrashpadDatabase)),
          mClient(new crashpad::CrashpadClient()),
          mConsentProvider(consentProvider()) {}

    // Gets a handle to single instance of crash reporter
    static CrashSystem* get();

    bool active() const { return mInitialized; }

    bool initialize() {
        if (mInitialized) {
            return false;
        }
        auto handler_path = ::base::FilePath(
                PathUtils::asUnicodePath(
                        System::get()
                                ->findBundledExecutable(kCrashpadHandler)
                                .data())
                        .c_str());
        if (handler_path.empty()) {
            dwarning("Crash handler not found, crash reporting disabled.");
        }
        auto database_path = ::base::FilePath(
                PathUtils::asUnicodePath(mDatabasePath.data()).c_str());
        auto metrics_path = ::base::FilePath();
        auto annotations = std::map<std::string, std::string>{
                {"prod", "AndroidEmulator"},
                {"ver", EMULATOR_FULL_VERSION_STRING}};
        bool active = mClient->StartHandler(handler_path, database_path,
                                            metrics_path, CrashURL, annotations,
                                            {"--no-rate-limit"}, true, true);

        mDatabase = crashpad::CrashReportDatabase::Initialize(database_path);
        mInitialized = active && mDatabase;

        dinfo("Storing crashdata in: %s, detection is %s for process: %d",
              mDatabasePath.c_str(), mInitialized ? "enabled" : "disabled",
              android::base::System::get()->getCurrentProcessId());

        if (mDatabase && mDatabase->GetSettings()) {
            mDatabase->GetSettings()->SetUploadsEnabled(false);
        }
        if (mInitialized) {
            CrashReporter::get()->initialize();
        }

        return mInitialized;
    }

    void uploadEntries() {
        if (!mDatabase || !mInitialized) {
            dinfo("Crash database unavailable, or not initialized, we will not "
                  "report any crashes.");
            return;
        }

        auto collect = mConsentProvider->consentRequired();
        bool areUploadsEnabled = (collect == CrashConsent::Consent::ALWAYS);
        if (mDatabase && areUploadsEnabled) {
            dinfo("Crash reports will be automatically uploaded to: %s",
                  CrashURL);
            mDatabase->GetSettings()->SetUploadsEnabled(areUploadsEnabled);
        }

        // Get consent for any report that was not yet uploaded.
        std::vector<crashpad::UUID> toRemove;
        std::vector<crashpad::CrashReportDatabase::Report> reports;
        std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
        mDatabase->GetCompletedReports(&reports);
        mDatabase->GetPendingReports(&pendingReports);
        reports.insert(reports.end(), pendingReports.begin(),
                       pendingReports.end());

        for (const auto& report : reports) {
            if (!report.uploaded) {
                if (mConsentProvider->requestConsent(report)) {
                    mDatabase->RequestUpload(report.uuid);
                    dinfo("Consent given for uploading crashreport %s to %s",
                          report.uuid.ToString().c_str(), CrashURL);
                } else {
                    dinfo("No consent for crashreport %s, deleting.",
                          report.uuid.ToString().c_str());
                    toRemove.push_back(report.uuid);
                }
            } else {
                toRemove.push_back(report.uuid);
                dinfo("Report %s is available remotely as: %s, deleting.",
                      report.uuid.ToString().c_str(), report.id.c_str());
            }
        }

        for (const auto& remove : toRemove) {
            mDatabase->DeleteReport(remove);
        }
    }

    void setConsentProvider(CrashConsent* replacement) {
        mConsentProvider.reset(replacement);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CrashSystem);

    std::string mDatabasePath;
    std::unique_ptr<crashpad::CrashpadClient> mClient;
    std::unique_ptr<crashpad::CrashReportDatabase> mDatabase;
    std::unique_ptr<CrashConsent> mConsentProvider;
    bool mInitialized{false};
};

LazyInstance<CrashSystem> sCrashSystem = LAZY_INSTANCE_INIT;

CrashSystem* CrashSystem::get() {
    return sCrashSystem.ptr();
}

bool inject_consent_provider(CrashConsent* myProvider) {
    auto crashSystem = CrashSystem::get();
    crashSystem->setConsentProvider(myProvider);
    return crashSystem->initialize();
}

}  // namespace crashreport
}  // namespace android

extern "C" {
using android::crashreport::CrashSystem;

bool crashhandler_init(int argc, char** argv) {
    if (!android::base::System::get()->getEnableCrashReporting()) {
        dinfo("Crashreporting disabled, not reporting crashes.");
        return false;
    }

    // Catch crashes in everything.
    // This promises to not launch any threads...
    if (!CrashSystem::get()->initialize()) {
        return false;
    }

    std::string arguments = "===== Command-line arguments =====\n";
    for (int i = 0; i < argc; i++) {
        arguments += argv[i];
        arguments += ' ';
    }
    arguments += "\n===== Environment =====\n";
    const auto allEnv = System::get()->envGetAll();
    for (const std::string& env : allEnv) {
        arguments += env;
        arguments += '\n';
    }

    android::crashreport::CrashReporter::get()->attachData(
            "command-line-and-environment.txt", arguments);

    // Make sure we don't report any hangs until all related loopers
    // actually get started.
    android::crashreport::CrashReporter::get()->hangDetector().pause(true);
    return true;
}

void upload_crashes(void) {
    const auto reporter = CrashSystem::get();
    if (reporter && reporter->active()) {
        reporter->uploadEntries();
    }
}
}
