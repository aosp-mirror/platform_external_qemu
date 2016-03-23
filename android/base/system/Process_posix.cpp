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

#include "android/base/EintrWrapper.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"

#include <chrono>
#include <memory>

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef __APPLE__
#include <spawn.h>
#endif  // __APPLE__

// This variable is a pointer to a zero-terminated array of all environment
// variables in the current process.
// Posix requires this to be declared as extern at the point of use
// NOTE: Apple developer manual states that this variable isn't available for
// the shared libraries, and one has to use the _NSGetEnviron() function instead
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern "C" char** environ;
#endif

namespace android {
namespace base {

using std::string;
using std::unique_ptr;
using std::vector;

Process::~Process() {
    if (mIsRunning) {
        auto waitResult = wait();
        if (waitResult != WaitResult::NoSuchProcess &&
            waitResult != WaitResult::Success) {
            LOG(ERROR) << "Leaked process |" << mCommandLine[0] << "|("
                       << mChildPid << ")";
        }
    }
}

bool Process::start() {
    if (mIsRunning) {
        return false;
    }
    sigset_t oldset;
    sigset_t set;
    if (sigemptyset(&set) || sigaddset(&set, SIGCHLD) ||
        pthread_sigmask(SIG_UNBLOCK, &set, &oldset)) {
        return false;
    }
    auto result = startPosix();
    pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
    return result;
}

static string formatCmd(const vector<string>& commandLine) {
    std::string cmd = "";
    if (LOG_IS_ON(VERBOSE)) {
        cmd = "|";
        for (const auto& param : commandLine) {
            cmd += param;
            cmd += " ";
        }
        cmd += "|";
    }
    return cmd;
}

bool Process::startPosix() {
    vector<char*> params;
    for (const auto& item : mCommandLine) {
        params.push_back(const_cast<char*>(item.c_str()));
    }
    params.push_back(nullptr);

#if defined(__APPLE__)
    pid_t pid = runViaPosixSpawn(mCommandLine[0].c_str(), params);
#else
    pid_t pid = runViaForkAndExec(mCommandLine[0].c_str(), params);
#endif  // !defined(__APPLE__)

    if (pid < 0) {
        LOG(VERBOSE) << "Failed to fork for command "
                     << formatCmd(mCommandLine);
        return false;
    }

    mIsRunning = true;
    mChildPid = pid;
    return true;
}

Process::WaitResult Process::wait(Process::ExitCode* outExitCode,
                                  Process::Duration timeoutMs) {
    if (!mIsRunning) {
        return WaitResult::NoSuchProcess;
    }

    // We were requested to wait for the process to complete.
    int exitCode;
    // Do not use SIGCHLD here because we're not sure if we're
    // running on the main thread and/or what our sigmask is.
    if (timeoutMs == kInfinite) {
        // Let's just wait forever and hope that the child process
        // exits.
        HANDLE_EINTR(waitpid(mChildPid, &exitCode, 0));
        if (outExitCode) {
            *outExitCode = WEXITSTATUS(exitCode);
        }
        mIsRunning = false;
        return WIFEXITED(exitCode) ? WaitResult::Success
                                   : WaitResult::UnknownError;
    }

    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();
    do {
        pid_t waitPid = HANDLE_EINTR(waitpid(mChildPid, &exitCode, WNOHANG));
        if (waitPid < 0) {
            auto local_errno = errno;
            LOG(VERBOSE) << "Error running command " << formatCmd(mCommandLine)
                         << ". waitpid failed with |" << strerror(local_errno)
                         << "|";
            mIsRunning = false;
            return WaitResult::UnknownError;
        }

        if (waitPid > 0) {
            if (outExitCode) {
                *outExitCode = WEXITSTATUS(exitCode);
            }
            mIsRunning = false;
            return WIFEXITED(exitCode) ? WaitResult::Success
                                       : WaitResult::UnknownError;
        }

        System::get()->sleepMs(10);
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
    } while (elapsed.count() < timeoutMs);
    return WaitResult::TimedOut;
}

void Process::terminate() {
    if (!mIsRunning) {
        return;
    }
    kill(mChildPid, SIGKILL);
    waitpid(mChildPid, nullptr, WNOHANG);
    mIsRunning = false;
}

pid_t Process::runViaForkAndExec(const char* command,
                                 const vector<char*>& params) {
    // If an output file was requested, open it before forking, since
    // creating a file in the child of a multi-threaded process is sketchy.
    //
    // It will be immediately closed in the parent process, and dup2'd into
    // stdout and stderr in the child process.
    int outputFd = 0;
    if ((mOptions & Options::DumpOutputToFile) != Options::None) {
        if (mOutputFile.empty()) {
            LOG(VERBOSE) << "Can not redirect output to empty file!";
            return -1;
        }

        // Ensure the umask doesn't get in the way while creating the
        // output file.
        mode_t old = umask(0);
        outputFd =
                open(mOutputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0700);
        umask(old);
        if (outputFd < 0) {
            LOG(VERBOSE) << "Failed to open file to redirect stdout/stderr";
            return -1;
        }
    }

    pid_t pid = fork();

    if (pid != 0) {
        if (outputFd > 0) {
            close(outputFd);
        }
        // Return the child's pid / error code to parent process.
        return pid;
    }

    // In the child process.
    // Do not do __anything__ except execve. That includes printing to
    // stdout/stderr. None of it is safe in the child process forked from a
    // parent with multiple threads.
    if ((mOptions & Options::DumpOutputToFile) != Options::None) {
        dup2(outputFd, 1);
        dup2(outputFd, 2);
        close(outputFd);
    } else if ((mOptions & Options::ShowOutput) == Options::None) {
        // We were requested to Options::HideAllOutput
        int fd = open("/dev/null", O_WRONLY);
        if (fd > 0) {
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
        }
    }
    if (execvp(command, params.data()) == -1) {
        // emulator doesn't really like exit calls from a forked process
        // (it just hangs), so let's just kill it
        if (raise(SIGKILL) != 0) {
            exit(RunFailed);
        }
    }
    // Should not happen, but let's keep the compiler happy
    return -1;
}

#if defined(__APPLE__)
pid_t Process::runViaPosixSpawn(const char* command,
                                const vector<char*>& params) {
    posix_spawnattr_t attr;
    if (posix_spawnattr_init(&attr)) {
        LOG(VERBOSE) << "Failed to initialize spawnattr obj.";
        return -1;
    }
    // Automatically destroy the successfully initialized attr.
    auto attrDeleter = [](posix_spawnattr_t* t) { posix_spawnattr_destroy(t); };
    unique_ptr<posix_spawnattr_t, decltype(attrDeleter)> scopedAttr(
            &attr, attrDeleter);

    if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT)) {
        LOG(VERBOSE) << "Failed to request CLOEXEC.";
        return -1;
    }

    posix_spawn_file_actions_t fileActions;
    if (posix_spawn_file_actions_init(&fileActions)) {
        LOG(VERBOSE) << "Failed to initialize fileactions obj.";
        return -1;
    }
    // Automatically destroy the successfully initialized fileActions.
    auto fileActionsDeleter = [](posix_spawn_file_actions_t* t) {
        posix_spawn_file_actions_destroy(t);
    };
    unique_ptr<posix_spawn_file_actions_t, decltype(fileActionsDeleter)>
            scopedFileActions(&fileActions, fileActionsDeleter);

    if ((mOptions & Options::DumpOutputToFile) != Options::None) {
        if (posix_spawn_file_actions_addopen(
                    &fileActions, 1, mOutputFile.c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC, 0700) ||
            posix_spawn_file_actions_addopen(
                    &fileActions, 2, mOutputFile.c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC, 0700)) {
            LOG(VERBOSE) << "Failed to redirect child output to file "
                         << mOutputFile;
            return -1;
        }
    } else if ((mOptions & Options::ShowOutput) != Options::None) {
        if (posix_spawn_file_actions_addinherit_np(&fileActions, 1) ||
            posix_spawn_file_actions_addinherit_np(&fileActions, 2)) {
            LOG(VERBOSE) << "Failed to request child stdout/stderr to be "
                            "left intact";
            return -1;
        }
    } else {
        if (posix_spawn_file_actions_addopen(&fileActions, 1, "/dev/null",
                                             O_WRONLY, 0700) ||
            posix_spawn_file_actions_addopen(&fileActions, 2, "/dev/null",
                                             O_WRONLY, 0700)) {
            LOG(VERBOSE) << "Failed to redirect child output to /dev/null";
            return -1;
        }
    }

    // Posix spawn requires that argv[0] exists.
    assert(params[0] != nullptr);

    pid_t pid;
    if (int error_code = posix_spawnp(&pid, command, &fileActions, &attr,
                                      params.data(), environ)) {
        LOG(VERBOSE) << "posix_spawnp failed: " << strerror(error_code);
        return -1;
    }
    return pid;
}
#endif  // defined(__APPLE__)

}  // namespace base
}  // namespace android
