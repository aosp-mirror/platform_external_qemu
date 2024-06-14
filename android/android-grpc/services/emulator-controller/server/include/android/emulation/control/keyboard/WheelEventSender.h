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
#include "aemu/base/async/Looper.h"
#include "android/emulation/control/keyboard/EventSender.h"
#include "android/skin/SwipeGesture.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

using android::base::Looper;

class Point {
public:
    Point() : mX(0), mY(0) {}
    Point(int x, int y) : mX(x), mY(y) {}
    int x() const { return mX; }
    int y() const { return mY; }

private:
    int mX;
    int mY;
};

using SwipeSimulator = android::emulation::control::SwipeGesture<Point>;

// Class that sends Mouse events on the current looper.
class WheelEventSender : public EventSender<WheelEvent> {
public:
    WheelEventSender(const AndroidConsoleAgents* const consoleAgents,
                     Looper* looper);
    ~WheelEventSender() = default;

protected:
    void doSend(const WheelEvent event) override;

private:

    bool mInputDeviceHasRotary;
    std::unique_ptr<SwipeSimulator> mSwipeSimulator;
    Looper* mLooper;
};

}  // namespace control
}  // namespace emulation
}  // namespace android