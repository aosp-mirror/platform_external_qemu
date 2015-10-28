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

#include "android/crashreport/CrashSystem.h"
#if defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#elif defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(_WIN32)
#include "client/windows/handler/exception_handler.h"
#else
#error Breakpad not supported on this platform
#endif

#include <memory>

namespace android {
namespace crashreport {

// Class |CrashReporter| is a singleton class that wraps breakpad OOP crash client.
// It provides functions to attach to a crash server and to wait for a crash
// server to start crash communication pipes.
class CrashReporter {
public:

    //Attach platform dependent crash handler.
    //Returns false if already attached or if attach fails.
    bool attachCrashHandler(const CrashSystem::CrashPipe& crashpipe);

    //Waits for a platform dependent pipe to become valid or timeout occurs.
    //Returns false if timeout occurs.
    bool waitServicePipeReady(const std::string& pipename,
                              const int timeout_ms);

    //Gets a handle to single instance of crash reporter
    static CrashReporter* get(void);

private:
    CrashReporter();
    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler;
    CrashReporter(CrashReporter const&) = delete;
    void operator=(CrashReporter const&) = delete;
};

}  // crashreport
}  // android
