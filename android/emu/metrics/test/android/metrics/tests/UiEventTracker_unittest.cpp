// Copyright 2016 The Android Open Source Project
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

#include "android/metrics/UiEventTracker.h"

#include <gtest/gtest.h>  // for Message, Suit...

#include <functional>  // for __base, function
#include <map>         // for map
#include <string>      // for string, basic...
#include <string_view>
#include <utility>     // for pair

#include "aemu/base/Log.h"                           // for base

#include "android/metrics/MetricsReporter.h"            // for MetricsReporter
#include "android/metrics/metrics.h"                    // for MetricsStopRe...
#include "android/metrics/tests/MockMetricsReporter.h"  // for MockMetricsRe...
#include "android/metrics/tests/MockMetricsWriter.h"    // for MockMetricsWr...
#include "studio_stats.pb.h"                            // for EmulatorUiEvent

using namespace android::base;
using namespace android::metrics;

static constexpr const char* kVersion = "version";
static constexpr const char* kFullVersion = "fullVersion";
static constexpr const char* kQemuVersion = "qemuVersion";
static constexpr const char* kSessionId = "session";

namespace android {
namespace metrics {

extern void set_unittest_Reporter(MetricsReporter::Ptr newPtr);

}
}  // namespace android

namespace {

class UiEventTrackerTest : public ::testing::Test {
public:
    std::shared_ptr<MockMetricsWriter> mWriter{
            std::make_shared<MockMetricsWriter>(kSessionId)};
    MockMetricsReporter* mReporter;

    void createReporter(bool enabled = true,
                        std::string_view ver = kVersion,
                        std::string_view fullVer = kFullVersion,
                        std::string_view qemuVer = kQemuVersion) {
        android::metrics::set_unittest_Reporter(
                std::make_unique<MockMetricsReporter>(enabled, mWriter, ver,
                                                      fullVer, qemuVer));
        mReporter = (MockMetricsReporter*)&MetricsReporter::get();
    }

    void expectDeliveryOf(std::map<std::string, int> expected) {
        mReporter->mOnReportConditional =
                [expected](MetricsReporter::ConditionalCallback cb) {
                    EXPECT_TRUE(bool(cb));
                    android_studio::AndroidStudioEvent event;
                    EXPECT_TRUE(cb(&event));
                    if (event.kind() == android_studio::AndroidStudioEvent::
                                                EMULATOR_UI_EVENTS) {
                        for (auto const& pair : expected) {
                            bool found = false;
                            auto key = pair.first;
                            auto val = pair.second;

                            for (int i = 0; i < event.emulator_ui_events_size();
                                 i++) {
                                auto elem = event.emulator_ui_events(i);
                                if (elem.element_id() == key) {
                                    found = true;
                                    EXPECT_EQ(elem.value(), val)
                                            << "expected " << key << " to have "
                                            << val << " values, not "
                                            << elem.value();
                                }
                            }

                            EXPECT_TRUE(found) << "did not find: " << key
                                               << " in delivered events.";
                        }
                    }
                };
    }
};

}  // namespace

TEST_F(UiEventTrackerTest, dropFirst) {
    createReporter();

    auto testTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB);
    testTracker->increment("FOO");
    EXPECT_EQ(0, testTracker->currentEvents().size());
}

TEST_F(UiEventTrackerTest, takeFirst) {
    createReporter();

    auto testTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB, false);
    testTracker->increment("FOO");
    EXPECT_EQ(1, testTracker->currentEvents().size());
}

TEST_F(UiEventTrackerTest, noDoubleRegistry) {
    // Make sure we don't create multiple reports if we add multiple elements.
    createReporter();

    auto testTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB);
    testTracker->increment("FOO");
    testTracker->increment("FOO");
    testTracker->increment("BAR");
    testTracker->increment("BAZ");
    EXPECT_EQ(1, testTracker->currentEvents().size());
}

TEST_F(UiEventTrackerTest, singleReport) {
    createReporter();

    // We should deliver a event that indicates that FOO was clicked 3 times
    // when the system shuts down.
    expectDeliveryOf({{"FOO", 3}});
    auto testTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB);
    {
        testTracker->increment("FOO");
        testTracker->increment("FOO");
        testTracker->increment("FOO");
        MetricsReporter::get().stop(MetricsStopReason::METRICS_STOP_GRACEFUL);
    }
}

TEST_F(UiEventTrackerTest, multipleReportsSameTracker) {
    createReporter();

    // We should deliver FOO and BAR
    expectDeliveryOf({{"FOO", 3}, {"BAR", 1}});
    auto testTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB);
    {
        testTracker->increment("FOO");
        testTracker->increment("FOO");
        testTracker->increment("BAR");
        MetricsReporter::get().stop(MetricsStopReason::METRICS_STOP_GRACEFUL);
    }
}

TEST_F(UiEventTrackerTest, multipleReportsMultipleTrackers) {
    createReporter();

    // We should deliver FOO and BAR, and they are tracked by different trackers
    // however they will end up in the same event..
    expectDeliveryOf({{"FOO", 3}, {"BAR", 1}});
    auto fooTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_BUG_TAB);
    auto barTracker = std::make_shared<UiEventTracker>(
            android_studio::EmulatorUiEvent::BUTTON_PRESS,
            android_studio::EmulatorUiEvent::EXTENDED_MIC_TAB);

    {
        fooTracker->increment("FOO");
        fooTracker->increment("FOO");
        barTracker->increment("BAR");
        MetricsReporter::get().stop(MetricsStopReason::METRICS_STOP_GRACEFUL);
    }
}
