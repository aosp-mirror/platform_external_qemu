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

#include "android/emulation/PropertyStore.h"
#include "android/emulation/PropertyWaiter.h"
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
    WaitObjectPtr waitAndroidProperty(
            const std::string& key,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback) override final;

    // Waits for an Android property and value to match asynchronously.
    // |key| - the property to wait for.
    // |value| - the property value to wait for.
    // |result_callback| - the callback function that will be invoked on the
    // calling thread after we get the property or timeout.
    WaitObjectPtr waitAndroidPropertyValue(
            const std::string& key,
            const std::string& value,
            std::function<void(const OptionalWaitObjectResult&)>
                    result_callback) override final;

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
}

AndroidPropertyInterfaceImpl::~AndroidPropertyInterfaceImpl() {
}

WaitObjectPtr AndroidPropertyInterfaceImpl::waitAndroidProperty(
        const std::string& key,
        std::function<void(const OptionalWaitObjectResult&)> result_callback) {
    auto command = std::shared_ptr<WaitObject>(
            new WaitObject(mLooper, result_callback, key));
    command->start();
    return command;
}

WaitObjectPtr AndroidPropertyInterfaceImpl::waitAndroidPropertyValue(
        const std::string& key,
        const std::string& value,
        std::function<void(const OptionalWaitObjectResult&)> result_callback) {
    auto command = std::shared_ptr<WaitObject>(
            new WaitObject(mLooper, result_callback, key, value));
    command->start();
    return command;
}

WaitObject::WaitObject(android::base::Looper* looper,
                       WaitObject::ResultCallback callback,
                       const std::string& key,
                       const std::string& value)
    : mLooper(looper),
      mResultCallback(callback),
      mKey(key),
      mValue(value),
      mFinished(false) {}

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
    std::shared_ptr<PropertyWaiter> waiter;
    bool res;

    if (mKey.empty()) {
        *result = {};
        return;
    }

    waiter.reset(new PropertyWaiter(mKey));
    if (!mValue.empty()) {
        waiter->setValue(mValue);
    }

    PropertyStore::get().addPropertyWaiter(waiter);
    // Block until the property store receives what we want.
    res = waiter->wait();

    if (res) {
        *result = android::base::makeOptional<WaitObjectResult>(
                {mKey, waiter->getValue()});
    } else {
        *result = {};
    }
}

}  // namespace emulation
}  // namespace android
