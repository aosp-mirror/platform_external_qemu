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
#include <cstdio>
#include <functional>
#include <future>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "android/base/streams/RingStreambuf.h"
#include "android/base/process/Process.h"

namespace android {
namespace base {

using android::base::streams::RingStreambuf;
using BufferDefinition = std::pair<size_t, std::chrono::milliseconds>;
using CommandArguments = std::vector<std::string>;

// A Command that you can execute and observe.
class Command {
public:
    using ProcessFactory =
            std::function<std::unique_ptr<ObservableProcess>(CommandArguments, bool)>;

    // Run the command with a std out buffer that can hold at most n bytes.
    // If the buffer is filled, the process will block for at most w ms before
    // timing out. Timeouts can result in data loss or stream closure.
    //
    // The default timeout is a year.
    Command& withStdoutBuffer(
            size_t n,
            std::chrono::milliseconds w = std::chrono::hours(24 * 365));

    // Run the command with a std err buffer that can hold at most n bytes.
    // If the buffer is filled, the process will block for at most w ms before
    // timing out. Timeouts can result in data loss or stream closure.
    //
    // The default timeout is a year.
    Command& withStderrBuffer(
            size_t n,
            std::chrono::milliseconds w = std::chrono::hours(24 * 365));

    // Adds a single argument to the list of arguments.
    Command& arg(const std::string& arg);

    // Adds a list of arguments to the existing arguments
    Command& args(const CommandArguments& args);

    // Launch the command as a deamon, you will not be able
    // to read stderr/stdout, the process will not bet terminated
    // when the created process goes out of scope.
    Command& asDeamon();

    // Launch the process
    std::unique_ptr<ObservableProcess> execute();

    // Create a new process
    static Command create(CommandArguments programWithArgs);

    // You likely only want to use this for testing..
    // Implement your own factory that produces an implemented process.
    // Make sure to set to nullptr when you want to revert to default.
    static void setTestProcessFactory(ProcessFactory factory);

protected:
    Command() = default;
    Command(CommandArguments args) : mArgs(args){};

private:
    static ProcessFactory sProcessFactory;
    static ProcessFactory sTestFactory;

    CommandArguments mArgs;
    bool mDeamon{false};
    bool mCaptureOutput{false};
    BufferDefinition mStdout{0, 0};
    BufferDefinition mStderr{0, 0};
};
}  // namespace base
}  // namespace android