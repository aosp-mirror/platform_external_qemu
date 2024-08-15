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
#include "aemu/base/process/Command.h"

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


#include "aemu/base/logging/Log.h"
#include "aemu/base/EintrWrapper.h"
#include "android/utils/exec.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                    \
    printf("%lld %s:%d %s| " fmt "\n",                                  \
           std::chrono::duration_cast<std::chrono::milliseconds>(       \
                   std::chrono::system_clock::now().time_since_epoch()) \
                   .count(),                                            \
           __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

#ifdef __APPLE__
#include <crt_externs.h>
#include <libproc.h>
#define environ (*_NSGetEnviron())
#else
#include <filesystem>
#endif

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif  // !_WIN32

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

#ifndef __APPLE__
static std::string read_proc_linux(int pid) {
    // parse out /proc/xx/cmdline
    std::string proc = "/proc/" + std::to_string(pid) + "/cmdline";
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
    return name;
}
#endif

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
    PosixProcess(bool deamon, bool inherit) : mDeamon(deamon) {
        mInherit = inherit;
    }

    ~PosixProcess() {
        if (mActions) {
            posix_spawn_file_actions_destroy(mActions);
        }
        if (mAttr) {
            posix_spawnattr_destroy(mAttr);
        }
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
        std::string name = read_proc_linux(mPid);
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
                                     bool captureOutput,
                                     bool replace) override {
        if (replace) {
            // Setup the arguments..
            std::vector<char*> args = toCharArray(cmdline);

            // The exec() functions only return if an error has occurred.
            safe_execv(args[0], args.data());
            return std::nullopt;
        }
        if (!mInherit) {
            mAttr = new posix_spawnattr_t;
            if (posix_spawnattr_init(mAttr)) {
                DD("Unable to initialize spawnattr..");
                return std::nullopt;
            }
#ifdef __APPLE__
            if (posix_spawnattr_setflags(mAttr, POSIX_SPAWN_CLOEXEC_DEFAULT)) {
                DD("Failed to request CLOEXEC.");
                return std::nullopt;
            }
#else
            // We need to mark all file handles as close on exec.
            int fdlimit = (int)sysconf(_SC_OPEN_MAX);
            DD("Marking %d as close on exec", fdlimit);
            for (int i = STDERR_FILENO + 1; i < fdlimit; i++) {
                int f = ::fcntl(i, F_GETFD);
                ::fcntl(i, F_SETFD, f | FD_CLOEXEC);
            }
            DD("Marked %d as close on exec -- done", fdlimit);
#endif
        }

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
            posix_spawn_file_actions_adddup2(action, mStdOutPipe[1], STDOUT_FILENO);
            posix_spawn_file_actions_adddup2(action, mStdErrPipe[1], STDERR_FILENO);

            posix_spawn_file_actions_addclose(action, mStdOutPipe[1]);
            posix_spawn_file_actions_addclose(action, mStdErrPipe[1]);
        } else {
            posix_spawn_file_actions_addopen(action, STDOUT_FILENO, "/dev/null", O_WRONLY, 0644);
            posix_spawn_file_actions_addopen(action, STDERR_FILENO, "/dev/null", O_WRONLY, 0644);
        }

        pid_t pid;
        auto error_code = posix_spawnp(&pid, cmdline[0].c_str(), action, mAttr,
                                       args.data(), environ);
        if (error_code) {
            derror("Unable to spawn process %s due to:, %s", cmdline[0],
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
    posix_spawnattr_t* mAttr{nullptr};
};

Command::ProcessFactory Command::sProcessFactory =
        [](CommandArguments args, bool deamon, bool inherit) {
            return std::make_unique<PosixProcess>(deamon, inherit);
        };

std::unique_ptr<Process> Process::fromPid(Pid pid) {
    return std::make_unique<PosixProcess>(pid);
}

std::unique_ptr<Process> Process::me() {
    return fromPid(getpid());
}

std::vector<std::unique_ptr<Process>> Process::fromName(std::string name) {
    std::vector<std::unique_ptr<Process>> processes;

#ifdef __APPLE__
    // Get list of all processes.
    int pid_array_size_needed = proc_listallpids(nullptr, 0);
    if (pid_array_size_needed <= 0) {
        return processes;
    }

    std::vector<pid_t> pid_array(pid_array_size_needed * 4);
    int pid_count = proc_listallpids(pid_array.data(),
                                     pid_array.size() * sizeof(pid_array[0]));
    if (pid_count <= 0) {
        return processes;
    }

    pid_array.resize(pid_count);

    // Ok, now we have an array of pids, and we just find the
    // one with the proper executable substring.
    for (const auto pid : pid_array) {
        char pname[PROC_PIDPATHINFO_MAXSIZE] = {0};
        proc_pidpath(pid, pname, sizeof(pname));
        if (strstr(pname, name.c_str())) {
            processes.push_back(Process::fromPid(pid));
        }
    }
#else
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        int pid = 0;
        if (std::sscanf(entry.path().c_str(), "/proc/%d", &pid) == 1) {
            if (read_proc_linux(pid).find(name) != std::string::npos) {
                processes.push_back(Process::fromPid(pid));
            }
        }
    }
#endif

    return processes;
}

}  // namespace base
}  // namespace android
