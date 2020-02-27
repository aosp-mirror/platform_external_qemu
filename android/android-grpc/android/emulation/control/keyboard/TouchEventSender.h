// Copyright (C) 2020 The Android Open Source Project
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
#include <unordered_set>      // for unordered_set

#include "android/console.h"  // for AndroidConsoleAgents

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace control {
class TouchEvent;

using base::Looper;

// Class that sends touch events on the current looper.
class TouchEventSender {
public:
    TouchEventSender(const AndroidConsoleAgents* const consoleAgents);
    ~TouchEventSender();
    bool send(const TouchEvent* request);
    bool sendOnThisThread(const TouchEvent* request);

private:
    void doSend(const TouchEvent touch);

    const AndroidConsoleAgents* const mAgents;
    std::unordered_set<int> mUsedSlots;
    Looper* mLooper;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
