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

#include <functional>

namespace android {
namespace base {

class Looper;

// A RecurrentTask is an object that allows you to run a task repeatedly on the
// event loop, until you're done.
// Example:
//
//     class AreWeThereYet {
//     public:
//         AreWeThereYet(Looper* looper) :
//                 mAskRepeatedly(looper,
//                                [this]() { return askAgain(); },
//                                1 * 60 * 1000) {}
//
//         bool askAgain() {
//             std::cout << "Are we there yet?" << std::endl;
//             return rand() % 2;
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
// It is thread safe though.
class RecurrentTask {
public:
    using TaskFunction = std::function<bool()>;
    using Duration = long long;

    RecurrentTask(Looper* looper,
                  TaskFunction function,
                  Duration taskIntervalMs);
    ~RecurrentTask();

    void start(bool runImmediately = false);
    void stopAsync();
    void stopAndWait();

    bool inFlight() const;
    void waitUntilRunning();
    Duration taskIntervalMs() const;

private:
    class Impl;
    Impl* mImpl;
};

}  // namespace base
}  // namespace android
