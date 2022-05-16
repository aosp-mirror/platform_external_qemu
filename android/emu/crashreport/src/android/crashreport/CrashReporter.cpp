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
#include "android/crashreport/CrashReporter.h"

#include <stdarg.h>  // for va_list, va_...
#include <stdio.h>   // for vsnprintf
#include <stdlib.h>  // for abort
#include <fstream>   // for basic_filebuf
#include <map>       // for map
#include <ostream>   // for operator<<
#include <string>    // for basic_string
#include <utility>   // for move
#include <vector>    // for vector<>::it...

#include "android/base/StringView.h"                     // for CStrWrapper
#include "android/base/files/PathUtils.h"                // for pj, PathUtils
#include "android/base/memory/LazyInstance.h"            // for LazyInstance
#include "android/base/system/System.h"                  // for System
#include "android/console.h"                             // for getConsoleAg...
#include "android/crashreport/SimpleStringAnnotation.h"  // for SimpleString...
#include "android/crashreport/crash-handler.h"           // for crashhandler...
#include "android/emulation/control/globals_agent.h"     // for QAndroidGlob...
#include "android/utils/debug.h"                         // for dinfo, derror
#include "android/version.h"                             // for EMULATOR_FUL...
#include "base/files/file_path.h"                        // for FilePath
#include "client/annotation.h"                           // for Annotation
#include "client/crash_report_database.h"                // for CrashReportD...
#include "client/crashpad_client.h"                      // for CrashpadClient
#include "client/settings.h"                             // for Settings
#include "util/misc/uuid.h"                              // for UUID

#ifdef _WIN32
#include <io.h>
#endif

using android::base::c_str;
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
const constexpr char kCrashOnExitFileName[] = "crash-on-exit";

// The crashpad handler binary, as shipped with the emulator.
const constexpr char kCrashpadHandler[] = "crashpad_handler";

using DefaultStringAnnotation = crashpad::StringAnnotation<1024>;
LazyInstance<CrashReporter> sCrashSystem = LAZY_INSTANCE_INIT;

CrashReporter::CrashReporter()
    : mDatabasePath(android::base::pj(System::get()->getTempDir(),
                                      kCrashpadDatabase)),
      mClient(new CrashpadClient()),
      mConsentProvider(consentProvider()) {}

void CrashReporter::AppendDump(const char* message) {
    passDumpMessage(message);
}

HangDetector& CrashReporter::hangDetector() {
    if (!mHangDetector) {
        mHangDetector = std::make_unique<HangDetector>([](auto message) {
            CrashReporter::get()->GenerateDumpAndDie(c_str(message));
        });
    }

    return *mHangDetector;
}
CrashReporter* CrashReporter::get() {
    return sCrashSystem.ptr();
}

void CrashReporter::GenerateDump(const char* message) {
    mAnnotationLog << message;
}

static const char* formatConsent(CrashConsent::Consent c) {
    constexpr const char* translate[] = {"ask to", "always", "never"};
    return translate[static_cast<int>(c)];
}

bool CrashReporter::initialize() {
    if (mInitialized) {
        return false;
    }
    auto handler_path = ::base::FilePath(
            PathUtils::asUnicodePath(
                    System::get()->findBundledExecutable(kCrashpadHandler))
                    .c_str());
    if (handler_path.empty()) {
        dwarning("Crash handler not found, crash reporting disabled.");
    }
    auto database_path =
            ::base::FilePath(PathUtils::asUnicodePath(mDatabasePath).c_str());
    auto metrics_path = ::base::FilePath();
    auto annotations = std::map<std::string, std::string>{
            {"prod", "AndroidEmulator"}, {"ver", EMULATOR_FULL_VERSION_STRING}};
    bool active = mClient->StartHandler(handler_path, database_path,
                                        metrics_path, CrashURL, annotations,
                                        {"--no-rate-limit"}, true, true);

    mDatabase = crashpad::CrashReportDatabase::Initialize(database_path);
    mInitialized = active && mDatabase;

    dinfo("Storing crashdata in: %s, detection is %s", mDatabasePath.c_str(),
          mInitialized ? "enabled" : "disabled");

    if (mDatabase && mDatabase->GetSettings()) {
        mDatabase->GetSettings()->SetUploadsEnabled(false);
    }

    return mInitialized;
}

void CrashReporter::GenerateDumpAndDie(const char* message) {
    passDumpMessage(message);
    // this is the most cross-platform way of crashing
    // any other I know about has its flaws:
    //  - abort() isn't caught by Breakpad on Windows
    //  - null() may screw the callstack
    //  - explicit *null = 1 can be optimized out
    //  - requesting dump and exiting later has a very noticable delay in
    //    between, so some real crash could stick in the middle
#ifndef __APPLE__
    volatile int* volatile ptr = nullptr;
    *ptr = 1313;  // die
#endif
    abort();  // make compiler believe it doesn't return
}

void CrashReporter::SetExitMode(const char* message) {
    mHangDetector->stop();
    attachData(kCrashOnExitFileName, message);
}

void CrashReporter::passDumpMessage(const char* message) {
    mAnnotationLog << message;
}

void CrashReporter::attachData(std::string name,
                               std::string data,
                               bool replace) {
    auto annotation = std::make_unique<SimpleStringAnnotation<256>>(name, data);
    mAnnotations.push_back(std::move(annotation));
}

void CrashReporter::attachBinaryData(std::string name,
                                     std::string data,
                                     bool replace) {
    dwarning("Binary data cannot be attached to a crash, ignoring %s",
             name.c_str());
}

void CrashReporter::uploadEntries() {
    if (!mDatabase || !mInitialized) {
        dinfo("Crash database unavailable, or not initialized, we will not "
              "report any crashes.");
        return;
    }

    auto collect = mConsentProvider->consentRequired();
    bool areUploadsEnabled = (collect == CrashConsent::Consent::ALWAYS);
    if (mDatabase && areUploadsEnabled) {
        dinfo("Crash reports will be automatically uploaded to: %s", CrashURL);
        mDatabase->GetSettings()->SetUploadsEnabled(areUploadsEnabled);
    }

    // Get consent for any report that was not yet uploaded.
    std::vector<crashpad::UUID> toRemove;
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    mDatabase->GetCompletedReports(&reports);
    mDatabase->GetPendingReports(&pendingReports);
    reports.insert(reports.end(), pendingReports.begin(), pendingReports.end());

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

}  // namespace crashreport
}  // namespace android

using android::crashreport::CrashReporter;

extern "C" {

bool crashhandler_init(int argc, char** argv) {
    if (!android::base::System::get()->getEnableCrashReporting()) {
        dinfo("Crashreporting disabled, not reporting crashes.")
        return false;
    }
    
    // Catch crashes in everything.
    // This promises to not launch any threads...
    if (!CrashReporter::get()->initialize()) {
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

void crashhandler_append_message(const char* message) {
    const auto reporter = CrashReporter::get();
    if (reporter && reporter->active()) {
        reporter->AppendDump(message);
    }
}

void crashhandler_append_message_format_v(const char* format, va_list args) {
    char message[2048] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);
    crashhandler_append_message(message);
}

void crashhandler_append_message_format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    crashhandler_append_message_format_v(format, args);
    va_end(args);
}

void crashhandler_die(const char* message) {
    const auto reporter = CrashReporter::get();
    if (reporter && reporter->active()) {
        derror("%s: fatal: %s", __func__, message);
        reporter->GenerateDumpAndDie(message);
    } else {
        dfatal("Emulator: exiting becase of the internal error '%s'", message);
        _exit(1);
    }
}

void __attribute__((noreturn))
crashhandler_die_format_v(const char* format, va_list args) {
    char message[2048] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);
    crashhandler_die(message);
}

void crashhandler_die_format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    crashhandler_die_format_v(format, args);
}

void crashhandler_add_string(const char* name, const char* string) {
    const auto reporter = CrashReporter::get();
    if (reporter && reporter->active()) {
        reporter->attachData(name, string);
    }
}

void crashhandler_exitmode(const char* message) {
    const auto reporter = CrashReporter::get();
    if (reporter && reporter->active()) {
        reporter->SetExitMode(message);
    }
}

void upload_crashes(void) {
    const auto reporter = CrashReporter::get();
    if (reporter && reporter->active()) {
        reporter->uploadEntries();
    }
}

bool crashhandler_copy_attachment(const char* destination, const char* source) {
    const auto reporter = CrashReporter::get();
    if (!(reporter && reporter->active())) {
        return false;
    }

    std::ifstream sourceFile(PathUtils::asUnicodePath(source).c_str(),
                             std::ios::binary);
    std::stringstream buffer;
    buffer << sourceFile.rdbuf();

    if (sourceFile.bad()) {
        return false;
    }
    reporter->attachData(destination, buffer.str());
    return true;
}

}  // extern "C"
