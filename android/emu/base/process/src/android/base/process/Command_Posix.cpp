// Copyright (C) 2022 The Android Open Source Project
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
#include "android/base/process/Command.h"

#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <climits>
#include <cstring>
#include <fstream>
#include <future>
#include <iosfwd>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "android/base/EintrWrapper.h"
#include "android/base/logging/CLog.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("%s:%d %s| " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

#ifdef __APPLE__
#include <crt_externs.h>
#include <libproc.h>
#define environ (*_NSGetEnviron())
#endif

namespace android {
namespace base {

std::vector<char*> toCharArray(const std::vector<std::string>& params) {
    std::vector<char*> args;
    for (const auto& param : params) {
        args.push_back(const_cast<char*>(param.c_str()));
    }
    args.push_back(nullptr);
    return args;
}

class PosixOverseer : public ProcessOverseer {
public:
    PosixOverseer(int stdOut[2], int stdErr[2]) {
        mStdOutPipe[0] = stdOut[0];
        mStdOutPipe[1] = stdOut[1];
        mStdErrPipe[0] = stdErr[0];
        mStdErrPipe[1] = stdErr[1];
    }

    ~PosixOverseer() { DD("~PosixOverseer"); }

    void readAndFlush(int fd, RingStreambuf* buffer) {
        char bytes[1024];
        auto bytes_read = read(fd, bytes, sizeof(bytes));
        int left = bytes_read;
        auto write = bytes;
        DD("read (%d): %ld: %s", fd, bytes_read,
           std::string(bytes, bytes_read).c_str());
        while (left > 0 && buffer->capacity() > 0) {
            auto toWrite = std::min<int>(buffer->capacity(), left);
            buffer->waitForAvailableSpace(toWrite);
            buffer->sputn(write, toWrite);
            write += toWrite;
            left -= toWrite;
        }
    }

    void start(RingStreambuf* out, RingStreambuf* err) override {
        int exit;
        std::vector<pollfd> plist = {{mStdOutPipe[0], POLLIN},
                                     {mStdErrPipe[0], POLLIN}};
        int bytes_read = -1;
        int rval;
        while ((rval = poll(&plist[0], plist.size(),
                            /*timeout*/ -1)) > 0) {
            DD("Event!");
            if (plist[0].revents & POLLIN) {
                // out..
                readAndFlush(mStdOutPipe[0], out);
            } else if (plist[1].revents & POLLIN) {
                readAndFlush(mStdErrPipe[0], err);
            }

            // Detect if we have closed the pipes.
            auto revents = plist[0].revents;
            if (revents & POLLHUP || revents & POLLNVAL || revents & POLLERR) {
                break;
            }

            revents = plist[1].revents;
            if (revents & POLLHUP || revents & POLLNVAL || revents & POLLERR) {
                break;
            }
        }

        // Push out any left overs.
        readAndFlush(mStdOutPipe[0], out);
        readAndFlush(mStdErrPipe[0], err);
        DD("Observer finished.");
    }

    // Cancel the observation of the process, no callbacks
    // should be invoked.
    // no writes to std_out, std_err should happen.
    void stop() override {
        close(mStdOutPipe[0]);
        close(mStdErrPipe[0]);
    };

private:
    int mStdOutPipe[2];
    int mStdErrPipe[2];
};

class PosixProcess : public ObservableProcess {
public:
    PosixProcess(Pid pid) : mDeamon(true) { mPid = pid; }
    PosixProcess(bool deamon) { mDeamon = deamon; }

    ~PosixProcess() {
        if (mActions)
            posix_spawn_file_actions_destroy(mActions);
        if (!mDeamon)
            PosixProcess::terminate();
    }

    bool terminate() override {
        using namespace std::chrono_literals;

        if (isAlive()) {
            kill(mPid, SIGKILL);
            HANDLE_EINTR(waitpid(mPid, nullptr, WNOHANG));
            wait_for_kernel(10s);
        }

        return !isAlive();
    }

    bool isAlive() const override {
        // Acknowledge process in case it is a zombie..
        getExitCode();
        return kill(mPid, 0) == 0;
    }

    std::future_status wait_for_kernel(
            const std::chrono::milliseconds timeout_duration) const override {
        using namespace std::chrono_literals;
        if (mPid == -1)
            return std::future_status::ready;

        auto wait_until = std::chrono::system_clock::now() + timeout_duration;
        while (std::chrono::system_clock::now() < wait_until && isAlive()) {
            std::this_thread::sleep_for(10ms);
            DD("Awakened, ready to check again.");
        }

        return std::chrono::system_clock::now() < wait_until
                       ? std::future_status::ready
                       : std::future_status::timeout;
    }

    std::string exe() const override {
#ifdef __APPLE__
        char name[PROC_PIDPATHINFO_MAXSIZE] = {0};
        proc_pidpath(mPid, name, sizeof(name));
#else
        // parse out /proc/xx/cmdline
        std::string proc = "/proc/" + std::to_string(mPid) + "/cmdline";
        DD("Looking for commandline in %s", proc.c_str());

        std::string name;
        FILE* proc_file;
        char ch;

        // Opening file in reading mode
        proc_file = fopen(proc.c_str(), "r");
        if (!proc_file) {
            DD("Warning, cannot find proc.");
            return "";
        }

        // /proc likely will not report file size, so read it!
        while ((ch = fgetc(proc_file)) != EOF) {
            if (ch == 0) {
                fclose(proc_file);
                return name;
            }
            name += ch;
        }
        fclose(proc_file);
#endif
        return name;
    }

    std::optional<ProcessExitCode> getExitCode() const override {
        if (mProcessExit.has_value()) {
            return mProcessExit;
        }

        ProcessExitCode exitCode;
        auto waitPid = HANDLE_EINTR(waitpid(mPid, &exitCode, WNOHANG));
        if (waitPid > 0)
            mProcessExit = WEXITSTATUS(exitCode);

        return mProcessExit;
    };

    std::optional<Pid> createProcess(const CommandArguments& cmdline,
                                     bool captureOutput) override {
        // Setup the arguments..
        std::vector<char*> args = toCharArray(cmdline);

        mActions = new posix_spawn_file_actions_t;
        auto action = mActions;
        posix_spawn_file_actions_init(action);

        if (captureOutput) {
            if (pipe(mStdOutPipe) || pipe(mStdErrPipe)) {
                dwarning("Unable to create pipes to connect to process: %s",
                         strerror(errno));
                return std::nullopt;
            }

            posix_spawn_file_actions_addclose(action, mStdOutPipe[0]);
            posix_spawn_file_actions_addclose(action, mStdErrPipe[0]);
            posix_spawn_file_actions_adddup2(action, mStdOutPipe[1], 1);
            posix_spawn_file_actions_adddup2(action, mStdErrPipe[1], 2);

            posix_spawn_file_actions_addclose(action, mStdOutPipe[1]);
            posix_spawn_file_actions_addclose(action, mStdErrPipe[1]);
        }

        pid_t pid;
        auto error_code = posix_spawnp(&pid, cmdline[0].c_str(), action, NULL,
                                       args.data(), environ);
        if (error_code) {
            derror("Unable to spawn process %s due to:, %s", cmdline[0].c_str(),
                   strerror(error_code));
            return std::nullopt;
        }

        if (captureOutput) {
            close(mStdOutPipe[1]);
            close(mStdErrPipe[1]);
        }

        return pid;
    }

    std::unique_ptr<ProcessOverseer> createOverseer() override {
        return std::make_unique<PosixOverseer>(mStdOutPipe, mStdErrPipe);
    }

    mutable std::optional<ProcessExitCode> mProcessExit;

    int mStdOutPipe[2];
    int mStdErrPipe[2];
    bool mDeamon{false};
    posix_spawn_file_actions_t* mActions{nullptr};
};

Command::ProcessFactory Command::sProcessFactory = [](CommandArguments args,
                                                      bool deamon) {
    return std::make_unique<PosixProcess>(deamon);
};

std::unique_ptr<Process> Process::fromPid(Pid pid) {
    return std::make_unique<PosixProcess>(pid);
}

std::unique_ptr<Process> Process::me() {
    return fromPid(getpid());
}

}  // namespace base
}  // namespace android
