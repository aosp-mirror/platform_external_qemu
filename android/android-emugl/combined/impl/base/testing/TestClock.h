// Copyright (C) 2022 The Android Open Source Project
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

#include <chrono>

namespace android {
namespace base {

// TestClock class for testing. Provides basic functions for advancing and resetting time.
// Satisfies requirements of TrivialClock: https://en.cppreference.com/w/cpp/named_req/TrivialClock
class TestClock {
   public:
    using rep = double;
    using period = std::ratio<1, 1>; /* TestClock::duration(1) is 1 second */
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<TestClock>;
    const bool is_steady = false;

    static time_point now() { return time_point(duration(getInternalTime())); }

    static void advance(double secondsPassed) { getInternalTime() += secondsPassed; }

    static void reset() { getInternalTime() = 0.0; }

   private:
    static double& getInternalTime() {
        static double internalTime = 0.0;
        return internalTime;
    }
};

}  // namespace base
}  // namespace android