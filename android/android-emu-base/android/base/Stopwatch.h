// Copyright 2017 The Android Open Source Project
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

#include "android/base/system/System.h"

#include <stdio.h>

namespace android {
namespace base {

class Stopwatch final {
public:
    Stopwatch();

    // Get the current elapsed time, microseconds.
    System::WallDuration elapsedUs() const;

    // Restart the stopwatch and return the current elapsed time, microseconds.
    System::WallDuration restartUs();

    static double sec(System::WallDuration us) { return us / 1000000.0; }
    static double ms(System::WallDuration us) { return us / 1000.0; }

private:
    System::WallDuration mStartUs;
};

// A class for convenient time tracking in a scope.
template <class Counter, int Divisor = 1>
class RaiiTimeTracker final {
    static_assert(Divisor > 0, "Bad divisor value");

    DISALLOW_COPY_ASSIGN_AND_MOVE(RaiiTimeTracker);

public:
    RaiiTimeTracker(Counter& time) : mTime(time) {}
    ~RaiiTimeTracker() { mTime += mSw.elapsedUs() / Divisor; }

private:
    Stopwatch mSw;
    Counter& mTime;
};

//
// A utility function to measure the time taken by the passed function |func|.
// Adds the elapsed time to the |time| parameter, returns exactly the same value
// as |func| returned.
// Usage:
//
//      int elapsedUs = 0;
//      measure(elapsedUs, [] { longRunningFunc1(); });
//      auto result = measure(elapsedUs, [&] { return longFunc2(); });
//      printf(..., elapsedUs);
//

template <class Counter, class Func>
auto measure(Counter& time, Func&& f) -> decltype(f()) {
    RaiiTimeTracker<Counter> rtt(time);
    return f();
}

template <class Counter, class Func>
auto measureMs(Counter& time, Func&& f) -> decltype(f()) {
    RaiiTimeTracker<Counter, 1000> rtt(time);
    return f();
}

// Some macros to have standard format of time measurements printed out.
#define STOPWATCH_PRINT_SPLIT(sw)                           \
    fprintf(stderr, "%s:%d %.03f ms\n", __func__, __LINE__, \
            (sw).restartUs() / 1000.0f);

#define STOPWATCH_PRINT(sw)                                       \
    fprintf(stderr, "%s:%d %.03f ms total\n", __func__, __LINE__, \
            (sw).elapsedUs() / 1000.0f);

}  // namespace base
}  // namespace android
