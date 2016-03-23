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

#include "android/base/system/Process.h"

namespace android {
namespace base {

using std::string;
using std::vector;

// static
bool Process::runCommand(const vector<string>& commandLine,
                         Process::Options options,
                         RunOptions runOptions,
                         const string& outputFile,
                         Process::Duration timeoutMs,
                         Process::ExitCode* outExitCode,
                         Process::Pid* outChildPid) {
    Process process(commandLine, options, outputFile);

    if (!process.start()) {
        return false;
    }

    if (outChildPid) {
        *outChildPid = process.getPid();
    }

    Process::WaitResult waitResult = process.wait(outExitCode, timeoutMs);
    if (waitResult == Process::WaitResult::Success) {
        return true;
    }
    if (waitResult == Process::WaitResult::TimedOut &&
        (runOptions & RunOptions::TerminateOnTimeout) != RunOptions::None) {
        process.terminate();
    }
    return false;
}

}  // namespace base
}  // namespace android
