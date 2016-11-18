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

// A RecurrentTask is an object that allows you to run a task repeatedly on the
// event loop, until you're done.
// Example:
//
//     class AreWeThereYet {
//     public:
//         AreWeThereYet(Looper* looper) :
//                 mAskRepeatedly(looper,
//                                std::bind(&AreWeThereYet::askAgain),
//                                1 * 60 * 1000) {}
//
//         void askAgain() {
//             std::cout << "Are we there yet?" << std::endl;
//         }
//
//         void startHike() {
//             mAskRepeatedly.start();
//         }
//
//     private:
//         RecurrentTask mAskRepeatedly;
//     };
//
// Note: RecurrentTask is meant to execute a task __on the looper thread__.
// It is not thread safe.
class RecurrentTask {
public:
    using TaskFunction = std::function<bool()>;

    RecurrentTask(Looper* looper,
                     TaskFunction function,
                     Looper::Duration taskIntervalMs) :
        mLooper(looper), mFunction(function), mTaskIntervalMs(taskIntervalMs),
        mTimer(mLooper->createTimer(&RecurrentTask::taskCallback, this)) {}

    ~RecurrentTask() {
        stop();
    }

    void start() {
        stop();
        mInFlight = true;
        mTimer->startRelative(mTaskIntervalMs);
    }

    void stop() {
        mInFlight = false;
        if(mTimer) {
            mTimer->stop();
        }
    }

    bool inFlight() {
        return mInFlight;
    }

protected:
    static void taskCallback(void* opaqueThis, Looper::Timer* timer) {
        auto thisPtr = static_cast<RecurrentTask*>(opaqueThis);
        if (!thisPtr->mFunction()) {
            thisPtr->mInFlight = false;
            return;
        }
        // It is possible that the client code in |mFunction| calls |stop|, so
        // we must double check before reposting the task.
        if (thisPtr->mInFlight) {
            thisPtr->mTimer->startRelative(thisPtr->mTaskIntervalMs);
        }
    }

private:
    Looper* mLooper;
    TaskFunction mFunction;
    Looper::Duration mTaskIntervalMs;
    std::unique_ptr<Looper::Timer> mTimer;
    bool mInFlight = false;
};

}  // namespace base
}  // namespace android
