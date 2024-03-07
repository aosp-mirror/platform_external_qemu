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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <cmath>
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/crashreport/SimpleStringAnnotation.h"
#include "android/emulation/control/globals_agent.h"
#include "android/utils/debug.h"
#include "android/version.h"
#include "base/files/file_path.h"
#include "client/annotation.h"
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "host-common/crash-handler.h"
#include "mini_chromium/base/files/file_path.h"
#include "util/misc/uuid.h"

#ifdef _WIN32
#include <io.h>
#else
#include <signal.h>
#endif

using android::base::c_str;
using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::System;
using base::FilePath;

namespace android {
namespace crashreport {

const constexpr char kCrashOnExitFileName[] = "crash-on-exit";
const constexpr char kCrashpadDatabase[] =
        "emu-crash-" EMULATOR_VERSION_STRING_SHORT ".db";

using DefaultStringAnnotation = crashpad::StringAnnotation<1024>;
LazyInstance<CrashReporter> sCrashReporter = LAZY_INSTANCE_INIT;

CrashReporter::CrashReporter() {}

FilePath CrashReporter::databaseDirectory() {
    auto crashDatabasePath =
            android::base::pj(System::get()->getTempDir(), kCrashpadDatabase);
    return FilePath(PathUtils::asUnicodePath(crashDatabasePath.data()).c_str());
}

bool CrashReporter::initialize() {
    mInitialized = true;
    return true;
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

static void enableSignalTermination() {
#if defined (__APPLE__) || defined (__linux__)
    // We will not get crash reports without the signals below enabled.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGFPE);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGSEGV);
    sigaddset(&set, SIGABRT);
    sigaddset(&set, SIGILL);
    int result = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    if (result != 0) {
        dwarning("Could not set thread sigmask: %d", result);
    }
#endif
}

void CrashReporter::GenerateDumpAndDie(const char* message) {
    // Make sure we can actually register a crash on this thread.
    enableSignalTermination();

    passDumpMessage(message);
    // this is the most cross-platform way of crashing
    // any other I know about has its flaws:
    //  - abort() isn't caught by Breakpad on Windows
    //  - null() may screw the callstack
    //  - explicit *null = 1 can be optimized out
    //  - requesting dump and exiting later has a very noticable delay in
    //    between, so some real crash could stick in the middle
    volatile int* volatile ptr = nullptr;
    *ptr = 1313;  // die

#ifndef _WIN32
    raise(SIGABRT);
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

    // Let's figure out how many bytes we need in our annotations.
    // We will bucketize by power of 2. We take the floor because
    // 2<<1 == 2^2, 2<<2 == 2^3, ..., etc..
    int shift = floor(log2(data.size()));

    std::unique_ptr<crashpad::Annotation> annotation;
    // Sadly we have to do some pseudo switching to minimize the use of
    // space in our minidump.
    if (shift <= 5)
        // 2<<5 == 2^6 == 64
        annotation = std::make_unique<SimpleStringAnnotation<2<<5>>(name, data);
    if (shift == 6)
        annotation = std::make_unique<SimpleStringAnnotation<2<<6>>(name, data);
    if (shift == 7)
        annotation = std::make_unique<SimpleStringAnnotation<2<<7>>(name, data);
    if (shift == 8)
        annotation = std::make_unique<SimpleStringAnnotation<2<<8>>(name, data);
    if (shift == 9)
        annotation = std::make_unique<SimpleStringAnnotation<2<<9>>(name, data);
    if (shift == 10)
        annotation = std::make_unique<SimpleStringAnnotation<2<<10>>(name, data);
    if (shift == 11)
        annotation = std::make_unique<SimpleStringAnnotation<2<<11>>(name, data);
    if (shift == 12)
        annotation = std::make_unique<SimpleStringAnnotation<2<<12>>(name, data);
    if (shift >= 13) {
        annotation = std::make_unique<SimpleStringAnnotation<2<<13>>(name, data);
        if (data.size() > 2<<13)
            dwarning(
                    "Crash annotation is very large (%d), only 16384 bytes "
                    "will be recorded, %d bytes are lost.",
                    data.size(), data.size() - (2<<13));
    }

    mAnnotations.push_back(std::move(annotation));
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
        dfatal("Emulator: exiting because of the internal error '%s'", message);
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
