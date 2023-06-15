
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
#include "android/emulation/control/keyboard/MouseEventSender.h"

#include "android/emulation/control/user_event_agent.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "host-common/feature_control.h"

namespace android {
namespace emulation {
namespace control {

void MouseEventSender::doSend(const MouseEvent request) {
    int buttonsState = request.buttons();
    // Keep sync the condition with |user_event_mouse| in
    // qemu-user-event-agent-impl.c
    if (feature_is_enabled(kFeature_VirtioInput) &&
        !feature_is_enabled(kFeature_VirtioMouse) &&
        !feature_is_enabled(kFeature_VirtioTablet)) {
        // Mask bits except for the first bit, because they are not
        // indicating mouse buttons for touch operations. (Instead
        // they are used for control flag like pause touch synching.)
        buttonsState &= 1;
    }
    mAgents->user_event->sendMouseEvent(request.x(), request.y(), 0,
                                        buttonsState, request.display());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
