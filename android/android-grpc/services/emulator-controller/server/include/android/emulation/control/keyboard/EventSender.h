// Copyright (C) 2023 The Android Open Source Project
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
#include "aemu/base/async/ThreadLooper.h"
#include "android/console.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
class EventSender {
public:
    EventSender(const AndroidConsoleAgents* const agents) : mAgents(agents) {}
    virtual ~EventSender() = default;

    // Sends the current event to the emulator over the UI thread.
    void send(const T event) {
        android::base::ThreadLooper::runOnMainLooper(
                [this, event] { doSend(event); });
    }

protected:
    virtual void doSend(const T event) = 0;
    const AndroidConsoleAgents* const mAgents;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
