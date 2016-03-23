// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/EnumFlags.h"
#include "android/base/system/Win32Utils.h"

#include <string>
#include <vector>

#include <limits.h>
#include <stdint.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#undef ERROR  // necessary to compile LOG(ERROR) statements
#else         // !_WIN32
#include <unistd.h>
#endif  // !_WIN32

namespace android {
namespace base {

class Process {
public:
    // We can't use System::Duration because System.h needs to include this
    // header.
    using Duration = int64_t;
    static const Duration kInfinite = -1;

#ifdef _WIN32
    using Pid = DWORD;
    using ExitCode = DWORD;
#else
    using Pid = pid_t;
    using ExitCode = int;
#endif

    enum class Options {
        None = 0,
        // Default = HideAllOutput
        Default = 0,

        HideAllOutput = 0,
        ShowOutput = 1,
        DumpOutputToFile = 2,
    };

    enum class RunOptions {
        None = 0,
        Default = 0,

        // Attempt to terminate the launched process if it doesn't finish in
        // time.
        // Note that terminating a mid-flight process can leave the whole system
        // in
        // a weird state.
        // Only make sense with |WaitForCompletion|.
        TerminateOnTimeout = 1,
    };

    // Return code from child process. Must be old-style enum so it can be cast
    // to int.
    enum { RunFailed = -1 };

    enum class WaitResult { Success, NoSuchProcess, TimedOut, UnknownError };

    Process(const std::vector<std::string>& commandLine,
            Options options = Options::Default,
            const std::string& outputFile = "")
        : mCommandLine(commandLine),
          mOptions(options),
          mOutputFile(outputFile) {}

    ~Process();

    bool start();

    // By default check for process exit and return immediately.
    WaitResult wait(ExitCode* outExitCode = nullptr, Duration timeoutMs = 0);

    // Handle with care.
    void terminate();

    // Only meaningful if |start| succeeded previously.
    Pid getPid() const { return mChildPid; }

    static bool runCommand(const std::vector<std::string>& commandLine,
                           Process::Options options = Process::Options::Default,
                           RunOptions runOptions = RunOptions::Default,
                           const std::string& outputFile = "",
                           Process::Duration timeoutMs = Process::kInfinite,
                           Process::ExitCode* outExitCode = nullptr,
                           Process::Pid* outChildPid = nullptr);

#ifndef _WIN32
protected:
    bool startPosix();
    pid_t runViaForkAndExec(const char* command,
                            const std::vector<char*>& params);
#ifdef __APPLE__
    pid_t runViaPosixSpawn(const char* command,
                           const std::vector<char*>& params);
#endif  // __APPLE__
#endif  //!_WIN32

private:
    const std::vector<std::string> mCommandLine;
    const Options mOptions;
    const std::string mOutputFile;

    Pid mChildPid;
#ifdef _WIN32
    Win32Utils::ScopedHandle mScopedProcessHandle;
#else
    bool mIsRunning = false;
#endif

    DISALLOW_COPY_AND_ASSIGN(Process);
};

}  // namespace base
}  // namespace android
