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

#include <functional>                    // for function
#include <iosfwd>                        // for istream
#include <memory>                        // for unique_ptr, enable_shared_fr...
#include <string>                        // for string
#include <vector>                        // for vector

#include "android/base/Compiler.h"       // for DISALLOW_COPY_ASSIGN_AND_MOVE
#include "android/base/Optional.h"       // for Optional
#include "android/base/StringView.h"     // for StringView
#include "android/base/system/System.h"  // for System, System::ProcessExitCode

namespace android {
namespace base {
class Looper;
}  // namespace base

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


    AdbCommandResult(android::base::System::ProcessExitCode exitCode,
                    std::istream* stream);

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
                                ResultCallback&& result_callback = [](const OptionalAdbCommandResult&){}
                               ) = 0;

    // Creates a new instance of the AdbInterface.
    static std::unique_ptr<AdbInterface> create(android::base::Looper* looper);
    // Creates global interface.
    static AdbInterface* createGlobal(android::base::Looper* looper);
    // createGlobal, but doesn't hang the UI thread for long running adb operations
    static AdbInterface* createGlobalOwnThread();
    static AdbInterface* getGlobal();
};


// Representation of an asynchronously running ADB command.
// These shouldn't be created directly, use AdbInterface::runAdbCommand.
class AdbCommand : public std::enable_shared_from_this<AdbCommand> {
public:
    ~AdbCommand() = default;

    using ResultCallback = AdbInterface::ResultCallback;

    // Returns true if the command is currently in the process of execution.
    virtual bool inFlight() const = 0;

    // Cancels the running command (has no effect if the command isn't running).
    virtual void cancel()  = 0;

    virtual void start(int checkTimeoutMs = 1000) = 0;
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
