// Copyright (C) 2024 The Android Open Source Project
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
#include <gtest/gtest.h>
#include <memory>
#include "android/avd/info.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/cmdline-definitions.h"
#include "android/console.h"
#include "android/emulation/control/keyboard/WheelEventSender.h"
#include "android/featurecontrol/testing/FeatureControlTest.h"
#include "android/skin/SwipeGesture.h"
#include "android/utils/looper.h"

namespace android {
namespace emulation {
namespace control {

namespace fc = android::featurecontrol;
using fc::Feature;

using android::base::TestLooper;

struct TestMouseEvent {
    int x;
    int y;
    int z;
    int buttons;
    int displayId;
};

struct TestWheelEvent {
    int dx;
    int dy;
    int displayId;
};

std::vector<TestMouseEvent> mouseEvents;
std::vector<TestWheelEvent> wheelEvents;
std::vector<int> rotaryEvents;

// Setup fake agents.
QAndroidUserEventAgent userAgent = {
        .sendMouseEvent =
                [](int x, int y, int z, int buttons, int displayId) {
                    mouseEvents.push_back({x, y, z, buttons, displayId});
                },
        .sendMouseWheelEvent =
                [](int dx, int dy, int displayId) {
                    wheelEvents.push_back({dx, dy, displayId});
                },
        .sendRotaryEvent = [](int x) { rotaryEvents.push_back(x); }};

// Hardware configuration with rotary enabled.
AndroidHwConfig s_hwConfig = {
        .hw_rotaryInput = 1,
};

QAndroidGlobalVarsAgent settings = {
        .hw = []() { return &s_hwConfig; },
};

QAndroidEmulatorWindowAgent emulatorWindow = {
        .getMultiDisplay = [](uint32_t id,
                              int32_t* x,
                              int32_t* y,
                              uint32_t* w,
                              uint32_t* h,
                              uint32_t* dpi,
                              uint32_t* flag,
                              bool* enable) -> bool {
            *w = 1024;
            *h = 768;
            *enable = true;
            return true;
        },
};

AndroidConsoleAgents agents = {
        .emu = &emulatorWindow,
        .user_event = &userAgent,
        .settings = &settings,
};

class WheelEventTest : public fc::FeatureControlTest {
public:
    void SetUp() override {
        FeatureControlTest::SetUp();
        for (const auto feature : android::featurecontrol::allFeatures()) {
            fc::setEnabledOverride(feature, false);
        }

        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
        android_registerMainLooper(reinterpret_cast<::Looper*>(
                static_cast<base::Looper*>(mLooper.get())));
        mouseEvents.clear();
        wheelEvents.clear();
        rotaryEvents.clear();
        wheelEventSender =
                std::make_unique<WheelEventSender>(&agents, mLooper.get());
    }

    void runLooper(int ms) {
        for (int i = 0; i < ms; i++) {
            auto now = testSystem.getHighResTimeUs();
            now += 1000;
            mLooper->runOneIterationWithDeadlineMs(now / 1000);
            testSystem.setUnixTimeUs(now);
        }
    }

    void TearDown() override {
        wheelEventSender.reset();
        android_registerMainLooper(nullptr);
        mLooper.reset();
    };

    void sendEvent(const WheelEvent& event) {
        wheelEventSender->send(event);
        runLooper(1000);
    }

protected:
    std::unique_ptr<WheelEventSender> wheelEventSender;
    std::unique_ptr<TestLooper> mLooper;
    WheelEvent defaultEvent;
    android::base::TestSystem testSystem{
            "progdir", base::System::kProgramBitness, "appdir"};
};

TEST_F(WheelEventTest, virtio_mouse_sends_wheel_event) {
    fc::setEnabledOverride(android::featurecontrol::VirtioMouse, true);

    defaultEvent.set_dx(20);
    defaultEvent.set_display(2);
    sendEvent(defaultEvent);

    ASSERT_EQ(wheelEvents.size(), 1);
    ASSERT_EQ(wheelEvents[0].dx, 20);
    ASSERT_EQ(wheelEvents[0].dy, 0);
    ASSERT_EQ(wheelEvents[0].displayId, 2);
}

TEST_F(WheelEventTest, virtio_mouse_no_side_effects) {
    fc::setEnabledOverride(android::featurecontrol::VirtioMouse, true);

    defaultEvent.set_dx(20);
    defaultEvent.set_display(2);
    sendEvent(defaultEvent);

    ASSERT_FALSE(wheelEvents.empty());
    ASSERT_TRUE(mouseEvents.empty());
    ASSERT_TRUE(rotaryEvents.empty());
}

TEST_F(WheelEventTest, rotary_sends_rotary_event) {
    fc::setEnabledOverride(android::featurecontrol::VirtioInput, true);
    wheelEventSender =
            std::make_unique<WheelEventSender>(&agents, mLooper.get());
    defaultEvent.set_dy(64);
    sendEvent(defaultEvent);

    ASSERT_TRUE(wheelEvents.empty());
    ASSERT_TRUE(mouseEvents.empty());
    ASSERT_EQ(rotaryEvents.size(), 1);
    ASSERT_EQ(rotaryEvents.back(), 8);
}

TEST_F(WheelEventTest, rotary_no_side_effects) {
    fc::setEnabledOverride(android::featurecontrol::VirtioInput, true);
    wheelEventSender =
            std::make_unique<WheelEventSender>(&agents, mLooper.get());
    defaultEvent.set_dy(64);

    sendEvent(defaultEvent);

    ASSERT_TRUE(wheelEvents.empty());
    ASSERT_TRUE(mouseEvents.empty());
    ASSERT_FALSE(rotaryEvents.empty());
}

TEST_F(WheelEventTest, no_virtio_no_side_effects) {
    defaultEvent.set_dx(20);
    defaultEvent.set_display(2);

    sendEvent(defaultEvent);
    ASSERT_TRUE(wheelEvents.empty());
    ASSERT_FALSE(mouseEvents.empty());
    ASSERT_TRUE(rotaryEvents.empty());
}

TEST_F(WheelEventTest, no_virtio_sends_emulated_events) {
    defaultEvent.set_dx(20);
    defaultEvent.set_display(2);

    sendEvent(defaultEvent);

    // We should have send a whole slew of events.
    ASSERT_GT(mouseEvents.size(), 20);

    // For display 2.
    for (const auto& event : mouseEvents) {
        EXPECT_EQ(event.displayId, 2);
    }
}

TEST_F(WheelEventTest, no_virtio_sends_emulated_horizontal_events) {
    defaultEvent.set_dx(20);
    sendEvent(defaultEvent);

    // We don't modify y axis
    for (int i = 1; i < mouseEvents.size(); i++) {
        EXPECT_EQ(mouseEvents[i].y, mouseEvents[i - 1].y);
    }
}

TEST_F(WheelEventTest, no_virtio_sends_emulated_vertical_events) {
    defaultEvent.set_dy(20);
    sendEvent(defaultEvent);

    // We don't modify x axis
    for (int i = 1; i < mouseEvents.size(); i++) {
        EXPECT_EQ(mouseEvents[i].x, mouseEvents[i - 1].x);
    }
}

TEST_F(WheelEventTest, no_virtio_sends_emulated_events_in_both_directions) {
    defaultEvent.set_dy(20);
    defaultEvent.set_dx(-20);
    sendEvent(defaultEvent);

    // We have moved around
    EXPECT_NE(mouseEvents.front().x, mouseEvents.back().x);
    EXPECT_NE(mouseEvents.front().y, mouseEvents.back().y);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
