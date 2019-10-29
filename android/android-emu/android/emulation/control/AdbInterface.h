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

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/async/Looper.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

using base::Optional;
using base::StringView;

class AdbCommand;
using AdbCommandPtr = std::shared_ptr<AdbCommand>;

// Encapsulates the result of running an ADB command.
struct AdbCommandResult {
    // Exit code returned by ADB.
    android::base::System::ProcessExitCode exit_code;

    // The command's output.
    // NOTE: we're using unique_ptr here because there's a bug in g++ 4 due to
    // which file streams are unmovable.
    std::unique_ptr<std::istream> output;

    AdbCommandResult(android::base::System::ProcessExitCode exitCode,
                     const std::string& outputName);

    ~AdbCommandResult();

private:
    std::string output_name;

    DISALLOW_COPY_ASSIGN_AND_MOVE(AdbCommandResult);
};
using OptionalAdbCommandResult = android::base::Optional<AdbCommandResult>;

// Class capable of finding all availabe adb executables.
class AdbLocator {
public:
    virtual ~AdbLocator() = default;
    virtual std::vector<std::string> availableAdb() = 0;

    virtual base::Optional<int> getAdbProtocolVersion(StringView adbPath) = 0;
};

class AdbDaemon {
public:
    virtual ~AdbDaemon() = default;
    virtual Optional<int> getProtocolVersion() = 0;
};

// A utility interface used to automatically locate the ADB binary and
// asynchronously run ADB commands.
class AdbInterface {
public:
    class Builder;

    virtual ~AdbInterface() noexcept = default;

    // Returns true is the ADB version is fresh enough.
    virtual bool isAdbVersionCurrent() const = 0;

    // Setup a custom adb path.
    virtual void setCustomAdbPath(const std::string& path) = 0;

    // Returns the automatically detected path to adb
    virtual const std::string& detectedAdbPath() const = 0;

    // Returns the path to adb binary
    virtual const std::string& adbPath() = 0;

    // Setup the port this interface is connected to
    virtual void setSerialNumberPort(int port) = 0;

    // Returns the serial string
    virtual const std::string& serialString() const = 0;

    // Runs an adb command asynchronously.
    // |args| - the arguments to pass to adb, i.e. "shell dumpsys battery"
    // |result_callback| - the callback function that will be invoked on the
    //                     calling thread after the command completes.
    // |timeout_ms| - how long to wait for the command to complete, in
    //                milliseconds.
    // |want_output| - if set to true, the argument passed to the callback will
    //                 contain the
    //                 output of the command.
    using ResultCallback = std::function<void(const OptionalAdbCommandResult&)>;

    virtual AdbCommandPtr runAdbCommand(const std::vector<std::string>& args,
                                        ResultCallback&& result_callback,
                                        base::System::Duration timeout_ms,
                                        bool want_output = true) = 0;

    // Queue a command to be sent over ADB.
    // If the command fails, it is retried until either it
    // succeeds or we give up.
    // The caller has no control over our "give up" criteria.
    // The caller can provide a callback that will receive
    // the results of the final attempt.
    virtual void enqueueCommand(const std::vector<std::string>& args,
                                void(*resultCallback)(const OptionalAdbCommandResult&) = nullptr
                               ) = 0;

    // Creates a new instance of the AdbInterface.
    static std::unique_ptr<AdbInterface> create(android::base::Looper* looper);
    // Creates global interface.
    static AdbInterface* createGlobal(android::base::Looper* looper);
    static AdbInterface* getGlobal();
};

class AdbInterfaceImpl;

// Representation of an asynchronously running ADB command.
// These shouldn't be created directly, use AdbInterface::runAdbCommand.
class AdbCommand : public std::enable_shared_from_this<AdbCommand> {
    friend android::emulation::AdbInterfaceImpl;

public:
    using ResultCallback = AdbInterface::ResultCallback;

    // Returns true if the command is currently in the process of execution.
    bool inFlight() const { return static_cast<bool>(mTask); }

    // Cancels the running command (has no effect if the command isn't running).
    void cancel() { mCancelled = true; }

private:
    AdbCommand(android::base::Looper* looper,
               const std::string& adb_path,
               const std::string& serial_string,
               const std::vector<std::string>& command,
               bool want_output,
               base::System::Duration timeoutMs,
               ResultCallback&& callback);
    void start(int checkTimeoutMs = 1000);
    void taskFunction(OptionalAdbCommandResult* result);
    void taskDoneFunction(const OptionalAdbCommandResult& result);

    android::base::Looper* mLooper;
    std::unique_ptr<base::ParallelTask<OptionalAdbCommandResult>> mTask;
    ResultCallback mResultCallback;
    std::string mOutputFilePath;
    std::vector<std::string> mCommand;
    bool mWantOutput;
    bool mCancelled = false;
    bool mFinished = false;
    int mTimeoutMs;
};

// Builder that can be used by unit tests to inject different behaviors,
// note that AdbInterface will take ownership of all pointers execpt
// the looper.
class AdbInterface::Builder {
public:
    Builder& setLooper(android::base::Looper* looper) {
        mLooper = looper;
        return *this;
    };
    // The builder will take ownership of the locator.
    Builder& setAdbLocator(AdbLocator* locator) {
        mLocator.reset(locator);
        return *this;
    }
    // The builder will take ownership of the daemon.
    Builder& setAdbDaemon(AdbDaemon* daemon) {
        mDaemon.reset(daemon);
        return *this;
    }

    std::unique_ptr<AdbInterface> build();

private:
    android::base::Looper* mLooper;
    std::unique_ptr<AdbLocator> mLocator;
    std::unique_ptr<AdbDaemon> mDaemon;
};

}  // namespace emulation
}  // namespace android
