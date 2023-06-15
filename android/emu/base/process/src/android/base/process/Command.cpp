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

#include "aemu/base/logging/CLog.h"

#include <stdio.h>
#include <cassert>
#include <climits>
#include <future>
#include <iterator>

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("%s:%d %F| " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace base {

ProcessExitCode Process::exitCode() const {
    auto status = wait_for(std::chrono::hours(24 * 365 * 10));
    return status == std::future_status::ready ? getExitCode().value_or(INT_MIN)
                                               : INT_MIN;
}

class ProcessOutputImpl : public ProcessOutput {
public:
    ProcessOutputImpl(std::unique_ptr<RingStreambuf> buffer)
        : mBuffer(std::move(buffer)), mStream(mBuffer.get()) {}

    std::string asString() override {
        return std::string(std::istreambuf_iterator<char>{asStream()}, {});
    }

    std::istream& asStream() override {
        mStream.clear();
        return mStream;
    }

    RingStreambuf* getBuf() { return mBuffer.get(); }

private:
    std::unique_ptr<RingStreambuf> mBuffer;
    std::istream mStream;
};

void ObservableProcess::runOverseer() {
    {
        std::unique_lock<std::mutex> lk(mOverseerMutex);
        dprint("Starting overseer to retrieve stderr/stdout of %s", exe().c_str());
        auto out =
                reinterpret_cast<ProcessOutputImpl*>(mStdOut.get())->getBuf();
        auto err =
                reinterpret_cast<ProcessOutputImpl*>(mStdErr.get())->getBuf();
        mOverseer->start(out, err);

        // Make sure we are really closed, and trigger any listeners.
        // (in case an overseer forgot)

        out->close();
        err->close();

        // Stop the overseer (likely a nop)
        mOverseer->stop();
        dprint("Stopped overseer");
        mOverseerActive = false;
    }
    mOverseerCv.notify_all();
}

std::future_status ObservableProcess::wait_for(
        const std::chrono::milliseconds timeout_duration) const {
    std::unique_lock<std::mutex> lk(mOverseerMutex);
    if (!mOverseerActive) {
        return wait_for_kernel(timeout_duration);
    }

    if (!mOverseerCv.wait_for(lk, timeout_duration,
                              [&] { return !mOverseerActive; }))
        return std::future_status::timeout;

    return std::future_status::ready;
}

void ObservableProcess::detach() {
    if (mOverseer)
        mOverseer->stop();
}

ObservableProcess::~ObservableProcess() {
    if (mOverseer)
        mOverseer->stop();
    if (mOverseerThread)
        mOverseerThread->join();
};

Command& Command::withStdoutBuffer(size_t n, std::chrono::milliseconds w) {
    assert(mDeamon == false);
    mStdout = {n, w};
    mCaptureOutput = true;
    return *this;
}

Command& Command::withStderrBuffer(size_t n, std::chrono::milliseconds w) {
    assert(mDeamon == false);
    mStderr = {n, w};
    mCaptureOutput = true;
    return *this;
}

// Adds a single argument to the list of arguments.
Command& Command::arg(const std::string& arg) {
    mArgs.push_back(arg);
    return *this;
}

// Adds a list of arguments to the existing arguments
Command& Command::args(const CommandArguments& args) {
    mArgs.insert(std::end(mArgs), std::begin(args), std::end(args));
    return *this;
}

Command& Command::asDeamon() {
    assert(mCaptureOutput == false);
    mDeamon = true;
    return *this;
}

Command& Command::inherit() {
    mInherit = true;
    return *this;
}

Command Command::create(std::vector<std::string> programWithArgs) {
    return Command(programWithArgs);
}

std::unique_ptr<ObservableProcess> Command::execute() {
    std::unique_ptr<ObservableProcess> proc;
    if (sTestFactory) {
        [[unlikely]] proc = sTestFactory(mArgs, mDeamon, mInherit);
    } else {
        proc = sProcessFactory(mArgs, mDeamon, mInherit);
    }

    // Connect I/O
    auto outbuf =
            std::make_unique<RingStreambuf>(mStdout.first, mStdout.second);
    proc->mStdOut = std::make_unique<ProcessOutputImpl>(std::move(outbuf));

    auto errbuf =
            std::make_unique<RingStreambuf>(mStderr.first, mStderr.second);
    proc->mStdErr = std::make_unique<ProcessOutputImpl>(std::move(errbuf));

    // Completion handlers.
    auto running = proc->createProcess(mArgs, mCaptureOutput);

    if (!running) {
        proc->mOverseer = std::unique_ptr<NullOverseer>();
        return proc;
    }
    proc->mPid = running.value();
    if (!mCaptureOutput) {
        proc->mOverseer = std::unique_ptr<NullOverseer>();
    } else {
        auto raw = proc.get();
        // TODO(jansene): Use condition_variable to assure that
        // overseer is really running after this call.
        proc->mOverseerActive = true;
        proc->mOverseer = proc->createOverseer();
        proc->mOverseerThread =
                std::make_unique<std::thread>([raw]() { raw->runOverseer(); });
    }

    return proc;
}


void Command::setTestProcessFactory(ProcessFactory factory) {
    sTestFactory = factory;
}

Command::ProcessFactory Command::sTestFactory = nullptr;

}  // namespace base
}  // namespace android