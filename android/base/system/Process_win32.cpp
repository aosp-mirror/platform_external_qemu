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

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"

#include <assert.h>

namespace android {
namespace base {

using std::string;
using std::vector;

Process::~Process() {}

bool Process::start() {
    if (mScopedProcessHandle) {
        return false;
    }

    auto commandLineCopy = mCommandLine;
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);

    if ((mOptions & Options::ShowOutput) == 0 ||
        ((mOptions & Options::DumpOutputToFile) != 0 && !mOutputFile.empty())) {
        startup.dwFlags = STARTF_USESHOWWINDOW;

        // 'Normal' way of hiding console output is passing null std handles
        // to the CreateProcess() function and CREATE_NO_WINDOW as a flag.
        // Sadly, in this case Cygwin runtime goes completely mad - its
        // whole FILE* machinery just stops working. E.g., resize2fs always
        // creates corrupted images if you try doing it in a 'normal' way.
        // So, instead, we do the following: run the command in a cmd.exe
        // with stdout and stderr redirected to either nul (for no output)
        // or the specified file (for file output).

        // 1. Find the commmand-line interpreter - which hides behind the
        // %COMSPEC% environment variable
        string comspec = System::get()->envGet("COMSPEC");
        if (comspec.empty()) {
            comspec = "cmd.exe";
        }

        // 2. Now turn the command into the proper cmd command:
        //   cmd.exe /C "command" "arguments" ... >nul 2>&1
        // This executes a command with arguments passed and redirects
        // stdout to nul, stderr is attached to stdout (so it also
        // goes to nul)
        commandLineCopy.insert(commandLineCopy.begin(), "/C");
        commandLineCopy.insert(commandLineCopy.begin(), comspec);

        if ((options & RunOptions::DumpOutputToFile) != RunOptions::None) {
            commandLineCopy.push_back(">");
            commandLineCopy.push_back(outputFile);
            commandLineCopy.push_back("2>&1");
        } else if ((options & RunOptions::ShowOutput) == RunOptions::None) {
            commandLineCopy.push_back(">nul");
            commandLineCopy.push_back("2>&1");
        }
    }

    PROCESS_INFORMATION pinfo = {};

    // this will point to either the commandLineCopy[0] or the executable
    // path found by the system
    StringView executableRef;
    // a buffer to store the executable path if we need to search for it
    string executable;
    if (PathUtils::isAbsolute(commandLineCopy[0])) {
        executableRef = commandLineCopy[0];
    } else {
        // try searching %PATH% and current directory for the binary
        const Win32UnicodeString name(commandLineCopy[0]);
        const Win32UnicodeString extension(PathUtils::kExeNameSuffix);
        Win32UnicodeString buffer(MAX_PATH);

        DWORD size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                                   buffer.size() + 1, buffer.data(), nullptr);
        if (size > buffer.size()) {
            // function may ask for more space
            buffer.resize(size);
            size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                                 buffer.size() + 1, buffer.data(), nullptr);
        }
        if (size == 0) {
            // Couldn't find anything matching the passed name
            return false;
        }
        if (buffer.size() != size) {
            buffer.resize(size);
        }
        executable = buffer.toString();
        executableRef = executable;
    }

    std::string args = executableRef;
    for (size_t i = 1; i < commandLineCopy.size(); ++i) {
        args += ' ';
        args += Win32Utils::quoteCommandLine(commandLineCopy[i]);
    }

    Win32UnicodeString commandUnicode(executableRef);
    Win32UnicodeString argsUnicode(args);

    if (!::CreateProcessW(
                commandUnicode.c_str(),  // program path
                argsUnicode.data(),  // command line args, has to be writable
                nullptr,             // process handle is not inheritable
                nullptr,             // thread handle is not inheritable
                FALSE,               // no, don't inherit any handles
                0,                   // default creation flags
                nullptr,             // use parent's environment block
                nullptr,             // use parent's starting directory
                &startup,            // startup info, i.e. std handles
                &pinfo)) {
        return false;
    }

    CloseHandle(pinfo.hThread);
    mScopedProcessHandle.reset(pinfo.hProcess);
    mChildPid = pinfo.dwProcessId;
    return true;
}

Process::WaitResult Process::wait(Process::ExitCode* outExitCode,
                                  Process::Duration timeoutMs) {
    if (!mScopedProcessHandle) {
        return WaitResult::NoSuchProcess;
    }

    if (timeoutMs == Process::kInfinite) {
        timeoutMs = INFINITE;
    }

    DWORD ret = ::WaitForSingleObject(mScopedProcessHandle.get(), timeoutMs);
    if (ret == WAIT_FAILED) {
        mScopedProcessHandle.reset();
        return WaitResult::UnknownError;
    } else if (ret == WAIT_TIMEOUT) {
        return WaitResult::TimedOut;
    }

    if (outExitCode) {
        DWORD exitCode;
        auto exitCodeSuccess =
                ::GetExitCodeProcess(mScopedProcessHandle.get(), &exitCode);
        assert(exitCodeSuccess);
        *outExitCode = exitCode;
    }
    mScopedProcessHandle.reset();
    return WaitResult::Success;
}

void Process::terminate() {
    if (!mScopedProcessHandle) {
        return;
    }
    ::TerminateProcess(mScopedProcessHandle.get(), 1);
    mScopedProcessHandle.reset();
}

}  // namespace base
}  // namespace android
