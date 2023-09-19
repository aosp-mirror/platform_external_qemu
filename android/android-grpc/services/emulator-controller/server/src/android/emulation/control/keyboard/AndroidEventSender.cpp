
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
#include "android/emulation/control/keyboard/AndroidEventSender.h"

#include "android/emulation/control/user_event_agent.h"
#include "android/skin/generic-event-buffer.h"

namespace android {
namespace emulation {
namespace control {

void AndroidEventSender::doSend(const AndroidEvent request) {
    SkinGenericEventCode event {
        .type = request.type(),
        .code = request.code(),
        .value = request.value(),
        .displayId = request.display()
    };
    mAgents->user_event->sendGenericEvent(event);
}
}  // namespace control
}  // namespace emulation
}  // namespace android