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

#include <unordered_map>                      // for unordered_map

#include "android/base/Log.h"                 // for LogStreamVoidify, LOG
#include "android/metrics/MetricsReporter.h"  // for MetricsReporter
#include "studio_stats.pb.h"                  // for EmulatorUiEvent, Androi...

namespace android {
namespace metrics {

UiEventTracker::UiEventTracker(
        ::android_studio::EmulatorUiEvent_EmulatorUiEventType eventType,
        ::android_studio::EmulatorUiEvent_EmulatorUiEventContext context)
    : mEventType(eventType), mContext(context) {}

void UiEventTracker::increment(const std::string& element, int amount) {
    if (mElementCounter.count(element) == 0) {
        // First time registering.. Report this metric on exit.
        registerCallback(element);
    }
    mElementCounter[element] += amount;
}

void UiEventTracker::registerCallback(const std::string& element) {
    auto weakSelf = std::weak_ptr<UiEventTracker>(shared_from_this());
    MetricsReporter::get().reportOnExit(
            [weakSelf, element](android_studio::AndroidStudioEvent* event) {
                if (const auto self = weakSelf.lock()) {
                    auto* metrics = event->mutable_emulator_ui_event();
                    metrics->set_element_id(element);
                    metrics->set_context(self->mContext);
                    metrics->set_type(self->mEventType);
                    metrics->set_value(self->mElementCounter[element]);
                    event->set_kind(android_studio::AndroidStudioEvent::
                                            EMULATOR_UI_EVENT);
                } else {
                    LOG(WARNING) << "Root tracker for metric " << element
                                 << " expired.";
                }
            });
}

}  // namespace metrics
}  // namespace android
