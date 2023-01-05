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

#include "aemu/base/files/PathUtils.h"                // for pj, PathUtils
#include "aemu/base/memory/LazyInstance.h"            // for LazyInstance
#include "android/base/system/System.h"                  // for System
#include "android/console.h"                             // for getConsoleAg...
#include "android/crashreport/SimpleStringAnnotation.h"  // for SimpleString...
#include "host-common/crash-handler.h"           // for crashhandler...
#include "android/emulation/control/globals_agent.h"     // for QAndroidGlob...
#include "android/utils/debug.h"                         // for dinfo, derror
#include "android/version.h"                             // for EMULATOR_FUL...
#include "client/annotation.h"                           // for Annotation
#include "client/crash_report_database.h"                // for CrashReportD...
#include "client/crashpad_client.h"                      // for CrashpadClient
#include "client/settings.h"                             // for Settings
#include "mini_chromium/base/files/file_path.h"          // for FilePath
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

const constexpr char kCrashOnExitFileName[] = "crash-on-exit";

using DefaultStringAnnotation = crashpad::StringAnnotation<1024>;
LazyInstance<CrashReporter> sCrashReporter = LAZY_INSTANCE_INIT;

CrashReporter::CrashReporter()
   {}

void CrashReporter::initialize() {
    mInitialized = true;
}

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
    return sCrashReporter.ptr();
}

void CrashReporter::GenerateDump(const char* message) {
    mAnnotationLog << message;
}

static const char* formatConsent(CrashConsent::Consent c) {
    constexpr const char* translate[] = {"ask to", "always", "never"};
    return translate[static_cast<int>(c)];
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



}  // namespace crashreport
}  // namespace android

using android::crashreport::CrashReporter;

extern "C" {


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
