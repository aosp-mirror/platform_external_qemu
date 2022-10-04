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
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "android/base/streams/RingStreambuf.h"

namespace android {
namespace base {

class Command;

using android::base::streams::RingStreambuf;
using CommandArguments = std::vector<std::string>;
using Pid = int;
using ProcessExitCode = int;

// A process in that is running in the os.
class Process {
public:
    virtual ~Process(){};

    // pid of the process, or -1 if it is invalid
    Pid pid() const { return mPid; };

    // Name of the process, note this might not be
    // immediately availabl.
    virtual std::string exe() const = 0;

    // The exit code of the process, this will block
    // and wait until the process has finished or detached.
    //
    // Calling this when isAlive() == true is will
    // return INT_MIN
    ProcessExitCode exitCode() const;

    // Unconditionally cause the process to exit. (Kill -9 ..)
    // Returns true if the process was terminated, false in case of failure.
    virtual bool terminate() = 0;

    // True if the pid is alive according to the os.
    virtual bool isAlive() const = 0;

    // Waits for process completion, returns if it is not finished for the
    // specified timeout duration
    virtual std::future_status wait_for(
            const std::chrono::milliseconds timeout_duration) const {
        return wait_for_kernel(timeout_duration);
    }

    // Waits for process completion, returns if it is not finished until
    // specified time point has been reached
    template <class Clock, class Duration>
    std::future_status wait_until(
            const std::chrono::time_point<Clock, Duration>& timeout_time)
            const {
        return wait_for(timeout_time - std::chrono::steady_clock::now());
    };

    bool operator==(const Process& rhs) const { return (mPid == rhs.mPid); }
    bool operator!=(const Process& rhs) const { return !operator==(rhs); }

    // Retrieve the object from the given pid id.
    static std::unique_ptr<Process> fromPid(Pid pid);

    // Retrieve myself.
    static std::unique_ptr<Process> me();

protected:
    // Return the exit code of the process, this should not block
    virtual std::optional<ProcessExitCode> getExitCode() const = 0;

    // Wait until this process is finished, by using an os level
    // call.
    virtual std::future_status wait_for_kernel(
            const std::chrono::milliseconds timeout_duration) const = 0;

    Pid mPid;
};

// Output of the process.
class ProcessOutput {
public:
    virtual ~ProcessOutput() = default;

    // Consumes the whole stream, and returns a string.
    virtual std::string asString() = 0;

    // Returns a stream, that will block until data
    // from the child is available.
    virtual std::istream& asStream() = 0;
};

// A process overseer is responsible for observering the process:
// It is  watching writes to std_err & std_out and passes them
// on to a RingStreambuf
class ProcessOverseer {
public:
    virtual ~ProcessOverseer() = default;

    // Start observering the process.
    //
    // The overseer should:
    //  - Write to std_out & std_err when needed.
    //  - Close the RingStreambuf when done with the stream.
    //  - Return when it can no longer read/write from stdout/stderr
    virtual void start(RingStreambuf* out, RingStreambuf* err) = 0;

    // Cancel the observation of the process if observation is still happening.
    // Basically this is used to detach from a running process.
    //
    // After return:
    //  - no writes to std_out, std_err should happen
    //  - all resource should be cleaned up.
    //  - a call to the start method should return with std::nullopt
    virtual void stop() = 0;
};

// A overseer that does nothing.
// Use this for detached processes, testing.
class NullOverseer : public ProcessOverseer {
public:
    virtual void start(RingStreambuf* out, RingStreambuf* err) override {}
    virtual void stop() override {}
};

// A running process that you can interact with.
//
// You obtain a process by running a command.
// For example:
//
// auto p = Command::create({"ls"}).execute();
// if (p->exitCode() == 0) {
//      auto list = p->out()->asString();
// }
class ObservableProcess : public Process {
public:
    // Kills the process..
    virtual ~ObservableProcess();

    // stdout from the child if not detached.
    ProcessOutput* out() { return mStdOut.get(); };

    // stderr from the child if not detached.
    ProcessOutput* err() { return mStdErr.get(); };

    // Detach the process overseer, and stop observing the process
    // you will:
    //  - Not able to read stdout/stderr
    //  - The process will not be terminated when going out of scope.
    void detach();

    std::future_status wait_for(
            const std::chrono::milliseconds timeout_duration) const override;

protected:
    // Subclasses should implement this to actually launch the process,
    // return std::nullopt in case of failure
    virtual std::optional<Pid> createProcess(const CommandArguments& args,
                                             bool captureOutput) = 0;

    // Create the overseer used to observe the process state.
    virtual std::unique_ptr<ProcessOverseer> createOverseer() = 0;

    // True if no overseer is needed
    bool mDeamon{false};

private:
    void runOverseer();

    std::unique_ptr<ProcessOverseer> mOverseer;
    std::unique_ptr<std::thread> mOverseerThread;
    bool mOverseerActive{false};
    mutable std::mutex mOverseerMutex;
    mutable std::condition_variable mOverseerCv;

    std::unique_ptr<ProcessOutput> mStdOut;
    std::unique_ptr<ProcessOutput> mStdErr;

    friend Command;
};
}  // namespace base
}  // namespace android
