// Copyright (C) 2019 The Android Open Source Project
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
#include <nlohmann/json.hpp>
#include "android/console.h"
#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"
#include "android/emulation/control/keyboard/TouchEventSender.h"

using json = nlohmann::json;

namespace android {
namespace emulation {
namespace control {

// An EventDispatcher can dispatch mouse,keyboard and touch events.
// It expects base64 protocol buffer definitions in the "msg" part of the json blob.
// The protocol buffers are defined in emulator_controller.proto.
class EventDispatcher {
public:
    EventDispatcher(const AndroidConsoleAgents* const agents);
    void dispatchEvent(const json msg);

private:
    const AndroidConsoleAgents* const mAgents;
    keyboard::EmulatorKeyEventSender mKeyEventSender;
    TouchEventSender mTouchEventSender;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
