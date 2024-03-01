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

#include <memory>  // for enable_shared_from_this
#include <mutex>
#include <string>         // for string, operator==, hash
#include <unordered_map>  // for unordered_map
#include <vector>

#include "aemu/base/Compiler.h"  // for DISALLOW_COPY_ASSIGN_...
#include "android/metrics/export.h"
#ifndef Q_MOC_RUN
// Qt 6.5.3 moc complains of a parsing error with this protobuf file. So exclude it from moc runs.
#include "studio_stats.pb.h"  // for EmulatorAutomatio...
#endif  // Q_MOC_RUN

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
// Note that all UiEvents will be batched and submitted in a single event
// when the metrics system shuts down under emulator_ui_events
class AEMU_METRICS_API UiEventTracker
    : public std::enable_shared_from_this<UiEventTracker> {
public:
    UiEventTracker(
            ::android_studio::EmulatorUiEvent_EmulatorUiEventType eventType,
            ::android_studio::EmulatorUiEvent_EmulatorUiEventContext context,
            bool dropFirst = true);

    void increment(const std::string& element, int amount = 1);
    std::vector<::android_studio::EmulatorUiEvent> currentEvents();

private:
    ::android_studio::EmulatorUiEvent_EmulatorUiEventType mEventType;
    ::android_studio::EmulatorUiEvent_EmulatorUiEventContext mContext;
    std::unordered_map<std::string, int> mElementCounter;
    std::mutex mElementLock;
    bool mDropFirst;
    bool mRegistered;
};

// This class is used to batch individual UiEventTrackers.
class MultipleUiEventsTracker {
public:
    DISALLOW_COPY_ASSIGN_AND_MOVE(MultipleUiEventsTracker);
    MultipleUiEventsTracker() = default;
    ~MultipleUiEventsTracker() = default;

    static MultipleUiEventsTracker& get();
    void registerTracker(std::shared_ptr<UiEventTracker> tracker);

private:
    std::mutex mCallbackLock;
    std::vector<std::shared_ptr<UiEventTracker>> mOnExit;
};

}  // namespace metrics
}  // namespace android
