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

#include "android/emulation/control/AndroidPropertyInterface.h"

#include "android/emulation/AndroidPropertyPipe.h"
#include "android/utils/debug.h"

#include <cstdio>
#include <fstream>
#include <string>

using android::base::Looper;
using android::base::ParallelTask;

namespace android {
namespace emulation {

class AndroidPropertyInterfaceImpl final : public AndroidPropertyInterface {
public:
    explicit AndroidPropertyInterfaceImpl(android::base::Looper* looper);
    ~AndroidPropertyInterfaceImpl();

    // Waits for an Android property asynchronously.
    // |key| - the property to wait for.
    // |result_callback| - the callback function that will be invoked on the
    // calling thread after we get the property or timeout.
    // |timeout_ms| - how long to wait for the property, in
    // milliseconds.
    WaitObjectPtr waitAndroidProperty(
            const std::string& key,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback,
            base::System::Duration timeout_ms) override final;

    // Waits for an Android property and value to match asynchronously.
    // |key| - the property to wait for.
    // |value| - the property value to wait for.
    // |result_callback| - the callback function that will be invoked on the
    // calling thread after we get the property or timeout.
    // |timeout_ms| - how long to wait for the property, in
    // milliseconds.
    WaitObjectPtr waitAndroidPropertyValue(
            const std::string& key,
            const std::string& value,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback,
            base::System::Duration timeout_ms) override final;

private:
    android::base::Looper* mLooper;
};

std::unique_ptr<AndroidPropertyInterface> AndroidPropertyInterface::create(
        android::base::Looper* looper) {
    return std::unique_ptr<AndroidPropertyInterface>{
            new AndroidPropertyInterfaceImpl(looper)};
}

AndroidPropertyInterfaceImpl::AndroidPropertyInterfaceImpl(
        android::base::Looper* looper)
    : mLooper(looper) {
    // Initializes the socket to wait on Android Properties.
    AndroidPropertyPipe::openConnection();
}

AndroidPropertyInterfaceImpl::~AndroidPropertyInterfaceImpl() {
    // Close the socket waiting on Android properties.
    AndroidPropertyPipe::closeConnection();
}

WaitObjectPtr AndroidPropertyInterfaceImpl::waitAndroidProperty(
        const std::string& key,
        std::function<void(const OptionalWaitObjectResult&)> result_callback,
        base::System::Duration timeout_ms) {
    auto command = std::shared_ptr<WaitObject>(
            new WaitObject(mLooper, timeout_ms, result_callback, key));
    command->start();
    return command;
}

WaitObjectPtr AndroidPropertyInterfaceImpl::waitAndroidPropertyValue(
        const std::string& key,
        const std::string& value,
        std::function<void(const OptionalWaitObjectResult&)> result_callback,
        base::System::Duration timeout_ms) {
    auto command = std::shared_ptr<WaitObject>(
            new WaitObject(mLooper, timeout_ms, result_callback, key, value));
    command->start();
    return command;
}

WaitObject::WaitObject(android::base::Looper* looper,
                       base::System::Duration timeout,
                       WaitObject::ResultCallback callback,
                       const std::string& key,
                       const std::string& value)
    : mLooper(looper),
      mResultCallback(callback),
      mTimeout(timeout),
      mFinished(false) {
    mKey = key;
    mValue = value;
}

void WaitObject::start(int checkTimeoutMs) {
    if (!mTask && !mFinished) {
        auto shared = shared_from_this();
        mTask.reset(new ParallelTask<OptionalWaitObjectResult>(
                mLooper,
                [shared](OptionalWaitObjectResult* result) {
                    shared->taskFunction(result);
                },
                [shared](const OptionalWaitObjectResult& result) {
                    shared->taskDoneFunction(result);
                },
                checkTimeoutMs));
        mTask->start();
    }
}

void WaitObject::taskDoneFunction(const OptionalWaitObjectResult& result) {
    if (!mCancelled) {
        mResultCallback(result);
    }
    mFinished = true;
    // This may invalidate this object and clean it up.
    // DO NOT reference any internal state from this class after this
    // point.
    mTask.reset();
}

void WaitObject::taskFunction(OptionalWaitObjectResult* result) {
    if (mKey.empty()) {
        *result = {};
        return;
    }
    std::string value;
    bool res;

    if (mValue.empty()) {
        res = AndroidPropertyPipe::waitAndroidProperty(mKey.c_str(), &value,
                                                       mTimeout);
    } else {
        res = AndroidPropertyPipe::waitAndroidPropertyValue(
                mKey.c_str(), mValue.c_str(), mTimeout);
    }

    if (res) {
        *result = android::base::makeOptional<WaitObjectResult>(
                {mKey, mValue.empty() ? value : mValue});
    } else {
        *result = {};
    }
}

}  // namespace emulation
}  // namespace android
