// Copyright (C) 2017 The Android Open Source Project
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

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class WaitObject;
using WaitObjectPtr = std::shared_ptr<WaitObject>;

// The key and value return from waiting for android property.
struct WaitObjectResult {
    std::string key;
    std::string value;
};
using OptionalWaitObjectResult = android::base::Optional<WaitObjectResult>;

// A utility interface used to wait for notifications/values from the guest.
class AndroidPropertyInterface {
public:
    virtual ~AndroidPropertyInterface() = default;

    // Waits for an Android property asynchronously.
    // |key| - the property to wait for.
    // |result_callback| - the callback function that will be invoked on the
    // calling thread after we get the property or timeout.
    // |timeout_ms| - how long to wait for the property, in
    // milliseconds.
    virtual WaitObjectPtr waitAndroidProperty(
            const std::string& key,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback,
            base::System::Duration timeout_ms) = 0;

    // Waits for an Android property and value to match asynchronously.
    // |key| - the property to wait for.
    // |value| - the property value to wait for.
    // |result_callback| - the callback function that will be invoked on the
    // calling thread after we get the property or timeout.
    // |timeout_ms| - how long to wait for the property, in
    // milliseconds.
    virtual WaitObjectPtr waitAndroidPropertyValue(
            const std::string& key,
            const std::string& value,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback,
            base::System::Duration timeout_ms) = 0;

    // Creates a new instance of the AndroidPropertyInterface.
    static std::unique_ptr<AndroidPropertyInterface> create(
            android::base::Looper* looper);
};

class AndroidPropertyInterfaceImpl;
// class TestAndroidPropertyInterface;

// Representation of asynchronously waiting on a property.
// These shouldn't be created directly, use
// AndroidPropertyInterface::waitAndroidProperty.
class WaitObject : public std::enable_shared_from_this<WaitObject> {
    friend android::emulation::AndroidPropertyInterfaceImpl;
    //    friend android::emulation::TestAndroidPropertyInterface;

public:
    using ResultCallback = std::function<void(const OptionalWaitObjectResult&)>;

    // Returns true if the command is currently in the process of execution.
    bool inFlight() const { return static_cast<bool>(mTask); }

    // Cancels the running command (has no effect if the command isn't running).
    void cancel() { mCancelled = true; }

private:
    WaitObject(android::base::Looper* looper,
               base::System::Duration timeout,
               ResultCallback callback,
               const std::string& key,
               const std::string& value = "");

    void start(int checkTimeoutMs = 1000);
    void taskFunction(OptionalWaitObjectResult* result);
    void taskDoneFunction(const OptionalWaitObjectResult& result);

    android::base::Looper* mLooper;
    std::unique_ptr<android::base::ParallelTask<OptionalWaitObjectResult>>
            mTask;
    ResultCallback mResultCallback;
    bool mCancelled = false;
    std::string mKey;
    std::string mValue;
    base::System::Duration mTimeout;
    bool mFinished;
};
}  // namespace emulation
}  // namespace android
