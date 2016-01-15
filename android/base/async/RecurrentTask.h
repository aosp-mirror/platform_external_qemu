// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"

#include <functional>
#include <memory>

namespace android {
namespace base {

template <class T>
class RecurrentTask {
public:
    using TaskFunction = std::function<bool(T*)>;

    // By default, run every minute.
    RecurrentTask<T>(Looper* looper,
                     TaskFunction function,
                     Looper::Duration taskIntervalMs = 60 * 1000) :
        mLooper(looper), mFunction(function), mTaskIntervalMs(taskIntervalMs) {}

    void start(T* context) {
        mContext = context;
        return scheduleTask();
    }

    void stop() {
        mContext = nullptr;
    }

protected:
    void scheduleTask() {
        mTimer.reset(mLooper->createTimer(&RecurrentTask<T>::taskCallback,
                                          this));
        mTimer->startRelative(mTaskIntervalMs);
    }

    static void taskCallback(void* opaqueThis, Looper::Timer* timer) {
        auto thisPtr = static_cast<RecurrentTask<T>*>(opaqueThis);
        if (thisPtr->mContext && thisPtr->mFunction(thisPtr->mContext)) {
            thisPtr->scheduleTask();
        }
    }

private:
    Looper* mLooper;
    TaskFunction mFunction;
    Looper::Duration mTaskIntervalMs;

    T* mContext = nullptr;
    std::unique_ptr<Looper::Timer> mTimer;
};

}  // namespace base
}  // namespace android
