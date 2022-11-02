// Copyright 2021 The Android Open Source Project
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

#include "aemu/base/EventNotificationSupport.h"

#include <gtest/gtest.h>            // for Message, SuiteApiResolver, TestPa...

#include "gtest/gtest_pred_impl.h"  // for Test, EXPECT_EQ, TEST

namespace android {
namespace base {

class SimpleEvent : public EventNotificationSupport<int> {
public:
    void setValue(int val) {
        mValue = val;
        fireEvent(val);
    }

    int mValue{0};
};

TEST(EventNotificationSupport, willFireEvent) {
    SimpleEvent evt;
    int val = 0;
    std::function<void(const int)> listener =
            [&val](int newVal) { val = newVal; };
    evt.addListener(&listener);
    evt.setValue(10);
    EXPECT_EQ(10, val);
}

TEST(EventNotificationSupport, raiUnregisters) {
    SimpleEvent evt;
    int val = 0;
    // No events are fired.
    evt.setValue(10);
    EXPECT_EQ(0, val);
    {
        // Should fire an event, as we have registed a listener
        RaiiEventListener<SimpleEvent, int> listener(
                &evt, [&val](const int newVal) { val = newVal; });
        evt.setValue(10);
        EXPECT_EQ(10, val);
    }

    // Listener has been cleaned up, so no events are fired.
    evt.setValue(100);
    EXPECT_EQ(10, val);
}

}  // namespace base
}  // namespace android
