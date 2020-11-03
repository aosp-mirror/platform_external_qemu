// Copyright 2020 The Android Open Source Project
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

#include <memory>             // for enable_shared_from_this
#include <string>             // for string, operator==, hash
#include <unordered_map>      // for unordered_map

#include "studio_stats.pb.h"  // for EmulatorUiEvent_EmulatorUiEventContext

namespace android {
namespace metrics {

// An UiEventTracker can be used to track simple counting events that need to be
// send when the emulator exits.
//
// Usage is straightforward:
//
// myTracker = std::make_shared<UiEventTracker>(eventType, context);
// myTracker->increment("foo")
//
// Make sure that myTracker is alive when the metrics are reported, otherwise nothing will happen.
class UiEventTracker : public std::enable_shared_from_this<UiEventTracker> {
public:
    UiEventTracker(
            ::android_studio::EmulatorUiEvent_EmulatorUiEventType eventType,
            ::android_studio::EmulatorUiEvent_EmulatorUiEventContext context);

    void increment(const std::string& element, int amount = 1);

private:
    void registerCallback(const std::string& element);

    ::android_studio::EmulatorUiEvent_EmulatorUiEventType mEventType;
    ::android_studio::EmulatorUiEvent_EmulatorUiEventContext mContext;
    std::unordered_map<std::string, int> mElementCounter;
};
}  // namespace metrics

}  // namespace android
