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

namespace android {
namespace crashreport {

// Class CrashReporter is a singleton class that wraps breakpad OOP crash
// client.
// It provides functions to attach to a crash server and to wait for a crash
// server to start crash communication pipes.
class CrashReporter {
public:
    static const int kWaitExpireMS = 500;
    static const int kWaitIntervalMS = 2;
    CrashReporter();

    virtual ~CrashReporter();

    // Attach platform dependent crash handler.
    // Returns false if already attached or if attach fails.
    virtual bool attachCrashHandler(
            const CrashSystem::CrashPipe& crashpipe) = 0;

    // Waits for a platform dependent pipe to become valid or timeout occurs.
    // Returns false if timeout occurs.
    virtual bool waitServicePipeReady(const std::string& pipename,
                                      int timeout_ms = kWaitExpireMS) = 0;

    // Special config when crash service is in child process
    virtual void setupChildCrashProcess(int pid) = 0;

    // returns dump dir
    const std::string& getDumpDir() const;

    // Gets a handle to single instance of crash reporter
    static CrashReporter* get();

private:
    DISALLOW_COPY_AND_ASSIGN(CrashReporter);
    const std::string mDumpDir;
};

}  // crashreport
}  // android
