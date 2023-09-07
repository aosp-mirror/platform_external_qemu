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
#include <chrono>
#include <unordered_map>

#include "android/emulation/control/keyboard/EventSender.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

// Class that sends Pen events on the current looper.
class PenEventSender : public EventSender<PenEvent> {
public:
    PenEventSender(const AndroidConsoleAgents* const consoleAgents)
        : EventSender<PenEvent>(consoleAgents){};
    ~PenEventSender() = default;

protected:
    void doSend(const PenEvent Pen) override;

private:
    // Last time external pen was used, we use this to expunge
    // dangling events
    std::chrono::seconds mIdLastUsedEpoch{LONG_MAX};

    // We expire Pen events after 120 seconds. This means that if
    // a given id has not received any updates in 120 seconds it will
    // be closed out upon receipt of the next event.
    const std::chrono::seconds kPEN_EXPIRE_AFTER_120S{120};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
