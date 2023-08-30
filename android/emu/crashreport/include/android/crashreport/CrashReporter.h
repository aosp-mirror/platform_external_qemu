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

#pragma once

#include <cassert>
#include <functional>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "aemu/base/Compiler.h"
#include "android/crashreport/AnnotationStreambuf.h"
#include "android/crashreport/CrashConsent.h"
#include "android/crashreport/HangDetector.h"
#include "base/files/file_path.h"
#include "client/annotation.h"
#include "host-common/crash-handler.h"


namespace crashpad {
class Annotation;
}  // namespace crashpad

using base::FilePath;

namespace android {
namespace crashreport {

using crashpad::Annotation;

// Class CrashReporter is a singleton class that wraps breakpad OOP crash
// client.
// It provides functions to attach to a crash server and to wait for a crash
// server to start crash communication pipes.
class CrashReporter {
public:
    using CrashCallback = std::function<void()>;
    CrashReporter();
    ~CrashReporter() = default;

    bool initialize();

    // returns dump dir
    const std::string& getDumpDir() const;

    // returns the directory for data exchange files. All files from this
    // directory will go to the reporting server together with the crash dump.
    // const std::string& getDataExchangeDir() const;

    // Gets a handle to single instance of crash reporter
    static CrashReporter* get();
    static void destroy();

    // Pass some data to the crash reporter, so in case of a crash it's uploaded
    // with the dump
    // |name| - a generic description of the data being added. Current
    //          implementation uploads the data in a file named |name|
    //          if |name| is empty the file gets some default generic name
    // |data| - a string of data to upload with the crash report
    // |replace| - replace all the data with the same name instead of appending
    void attachData(std::string name, std::string data, bool replace = false);


    // To make it easier to diagnose general issues,
    // have a function to append to the dump message file
    // without needing to generate a minidump
    // or crash the emulator.
    void AppendDump(const char* message);
    // The following two functions write a dump of current process state.
    // Both pass the |message| to the dump writer, so it is sent together with
    // the dump file
    // GenerateDumpAndDie() also doesn't return - it terminates process in a
    // fastest possible way. The process doesn't show/print any message to the
    // user with the possible exception of "Segmentation fault".
    void GenerateDump(const char* message);

    // This crashes the system..
    ANDROID_NORETURN void GenerateDumpAndDie(const char* message);

    void SetExitMode(const char* message);

    // Aks consent for every entry, and upload if requested.
    void uploadEntries();

    HangDetector& hangDetector();

    bool active() const { return mInitialized; }

    static FilePath databaseDirectory();

private:
    // Include the |message| as an annotation
    void passDumpMessage(const char* message);

private:
    DISALLOW_COPY_AND_ASSIGN(CrashReporter);

    std::unique_ptr<HangDetector> mHangDetector;
    std::vector<std::unique_ptr<Annotation>> mAnnotations;
    DefaultAnnotationStreambuf mAnnotationBuf{"internal-msg"};
    std::ostream mAnnotationLog{&mAnnotationBuf};
    bool mInitialized{false};
};

}  // namespace crashreport
}  // namespace android
