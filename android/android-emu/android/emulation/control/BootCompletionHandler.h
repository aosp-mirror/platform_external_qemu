// Copyright 2023 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once
#include <chrono>

#include "aemu/base/EventNotificationSupport.h"

namespace android {
namespace emulation {
namespace control {

using android::base::EventNotificationSupport;

using namespace std::chrono_literals;
using std::chrono::milliseconds;

struct BootCompletedEvent {
    milliseconds time;
    bool mBootCompleted;
};

class BootCompletionHandler
    : public EventNotificationSupport<BootCompletedEvent> {
public:
    void signalBootChange(bool booted) {
        mBooted = booted;
        BootCompletedEvent event{.time = mTime, .mBootCompleted = mBooted};
        fireEvent(event);
    };

    void setBootTime(milliseconds time) { mTime = time; };
    milliseconds bootTime() { return mTime; };
    bool hasBooted() { return mBooted; }

    static BootCompletionHandler* get();
private:
    milliseconds mTime{0};
    bool mBooted{false};
};

}  // namespace control
}  // namespace emulation
}  // namespace android