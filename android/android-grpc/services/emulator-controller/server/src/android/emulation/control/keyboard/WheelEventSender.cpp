
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
#include "android/emulation/control/keyboard/WheelEventSender.h"

#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/control/user_event_agent.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "host-common/feature_control.h"

namespace android {
namespace emulation {
namespace control {

WheelEventSender::WheelEventSender(
        const AndroidConsoleAgents* const consoleAgents)
    : EventSender<WheelEvent>(consoleAgents) {
    auto settings = mAgents->settings;
    mInputDeviceHasRotary =
            feature_is_enabled(kFeature_VirtioInput) &&
            (settings->hw()->hw_rotaryInput ||
             (settings->avdInfo() &&
              avdInfo_getAvdFlavor(settings->avdInfo()) == AVD_WEAR));
}

void WheelEventSender::doSend(const WheelEvent event) {
    auto agent = mAgents->user_event;
    if (mInputDeviceHasRotary) {
        // dy() is pre-multiplied by 120 (pixels to
        // scroll per click), and we need the rotation
        // angle (15 per wheel click), so we need to
        // scale by (15 / 120) == 1/8.
        agent->sendRotaryEvent(event.dy() / 8);
    } else {
        agent->sendMouseWheelEvent(event.dx(), event.dy(), event.display());
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
