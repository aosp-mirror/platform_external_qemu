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
#include "android/metrics/UiEventTracker.h"

#include <functional>     // for __base
#include <unordered_map>  // for __hash_map_iterator
#include <utility>        // for pair

#include "aemu/base/memory/LazyInstance.h"  // for LazyInstance
#include "android/metrics/MetricsReporter.h"   // for MetricsReporter
#include "studio_stats.pb.h"                   // for EmulatorUiEvent, Andro...

namespace android {
namespace metrics {

UiEventTracker::UiEventTracker(
        ::android_studio::EmulatorUiEvent_EmulatorUiEventType eventType,
        ::android_studio::EmulatorUiEvent_EmulatorUiEventContext context,
        bool dropFirst)
    : mEventType(eventType),
      mContext(context),
      mDropFirst(dropFirst),
      mRegistered(false) {}

void UiEventTracker::increment(const std::string& element, int amount) {
    const std::lock_guard<std::mutex> lock(mElementLock);
    if (!mRegistered) {
        MultipleUiEventsTracker::get().registerTracker(shared_from_this());
        mRegistered = true;
    }
    if (mElementCounter.count(element) == 0) {
        if (mDropFirst) {
            mElementCounter[element] -= amount;
        }
    }
    mElementCounter[element] += amount;
}

std::vector<::android_studio::EmulatorUiEvent> UiEventTracker::currentEvents() {
    const std::lock_guard<std::mutex> lock(mElementLock);
    std::vector<::android_studio::EmulatorUiEvent> events;
    for (auto it = mElementCounter.begin(); it != mElementCounter.end(); ++it) {
        // We only report elements that have an actual value.
        if (it->second > 0) {
            ::android_studio::EmulatorUiEvent event;
            event.set_element_id(it->first);
            event.set_context(mContext);
            event.set_type(mEventType);
            event.set_value(it->second);
            events.push_back(event);
        }
    }
    return events;
}

static base::LazyInstance<MultipleUiEventsTracker> sTrackerInstance = {};

MultipleUiEventsTracker& MultipleUiEventsTracker::get() {
    return sTrackerInstance.get();
}

void MultipleUiEventsTracker::registerTracker(
        std::shared_ptr<UiEventTracker> tracker) {
    const std::lock_guard<std::mutex> lock(mCallbackLock);

    // We only need to register a callback once!
    if (mOnExit.empty()) {
        MetricsReporter::get().reportOnExit(
                [=](android_studio::AndroidStudioEvent* event) {
                    const std::lock_guard<std::mutex> lock(mCallbackLock);
                    for (auto const& tracker : mOnExit) {
                        for (auto const& currentEvt :
                             tracker->currentEvents()) {
                            auto newEvent = event->add_emulator_ui_events();
                            *newEvent = currentEvt;
                        }
                    }

                    event->set_kind(android_studio::AndroidStudioEvent::
                                            EMULATOR_UI_EVENTS);
                });
    }
    mOnExit.push_back(tracker);
}
}  // namespace metrics
}  // namespace android
