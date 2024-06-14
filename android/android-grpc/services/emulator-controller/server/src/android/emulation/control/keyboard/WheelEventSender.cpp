
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

#include "aemu/base/async/RecurrentTask.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/skin/event.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "host-common/feature_control.h"

namespace android {
namespace emulation {
namespace control {

WheelEventSender::WheelEventSender(
        const AndroidConsoleAgents* const consoleAgents,
        Looper* looper)
    : EventSender<WheelEvent>(consoleAgents), mLooper(looper) {
    auto settings = mAgents->settings;
    mInputDeviceHasRotary =
            feature_is_enabled(kFeature_VirtioInput) &&
            (settings->hw()->hw_rotaryInput ||
             (settings->avdInfo() &&
              avdInfo_getAvdFlavor(settings->avdInfo()) == AVD_WEAR));
}


void WheelEventSender::doSend(const WheelEvent event) {
    const bool inputDeviceHasWheel =
            android::featurecontrol::isEnabled(
                    android::featurecontrol::VirtioMouse) ||
            android::featurecontrol::isEnabled(
                    android::featurecontrol::VirtioTablet);

    if (event.dy() == 0 && event.dx() == 0) {
        dwarning("Wheel event with zero delta, ignoring");
        return;
    }

    auto agent = mAgents->user_event;
    if (mInputDeviceHasRotary) {
        // dy() is pre-multiplied by 120 (pixels to
        // scroll per click), and we need the rotation
        // angle (15 per wheel click), so we need to
        // scale by (15 / 120) == 1/8.
        agent->sendRotaryEvent(event.dy() / 8);
    } else if (inputDeviceHasWheel) {
        agent->sendMouseWheelEvent(event.dx(), event.dy(), event.display());
    } else {
        // Emulation time!
        // Scroll 1/9 of window height per a wheel click.
        uint32_t width, height;
        const int displayId = event.display();
        bool enabled = true;
        const bool multiDisplayQueryWorks = mAgents->emu->getMultiDisplay(
                displayId, nullptr, nullptr, &width, &height, nullptr, nullptr,
                &enabled);

        // b/248394093 Initialize the value of width and height to the
        // default hw settings regardless of the display ID when multi
        // display agent doesn't work.
        if (!multiDisplayQueryWorks) {
            width = mAgents->settings->hw()->hw_lcd_width;
            height = mAgents->settings->hw()->hw_lcd_height;
        }
        const auto pos = Point(width / 2, height / 2);
        int scaledDeltaX = 0;
        int scaledDeltaY = 0;

        if (event.dy() != 0) {
            const int dy = event.dy() * 120 / 15;
            scaledDeltaY = height * dy / 120 / 9;
        }
        if (event.dx() != 0) {
            const int dx = event.dx() * 120 / 15;
            scaledDeltaX = width * dx / 120 / 9;
        }
        const SwipeSimulator::Vector2d scaledDelta{scaledDeltaX, scaledDeltaY};

        if (!mSwipeSimulator || mSwipeSimulator->atEnd()) {
            // Start the gesture if it's not yet active, or it has expired.
            mSwipeSimulator = std::make_unique<SwipeSimulator>(
                    mLooper, mAgents->user_event, displayId, pos, scaledDelta);
            return;
        }

        const auto swiped = mSwipeSimulator->swiped();
        if (abs(swiped.x()) >= width / 16 || abs(swiped.y()) >= height / 16) {
            // Reposition the gesture to avoid the fake touch point from
            // going outside of the window.
            mSwipeSimulator->reposition(pos, scaledDelta);
        } else {
            mSwipeSimulator->moveMore(scaledDelta);
        }
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
