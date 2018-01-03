// Copyright 2018 The Android Open Source Project
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

#include "android/base/Stopwatch.h"

#include <array>
#include <atomic>
#include <utility>

namespace android {
namespace snapshot {


//
//   IncrementalStats - a class to track incremental snapshot
// statistics.
//
// Tracking is only enabled for SNAPSHOT_PROFILE > 1, otherwise
// it won't do anything more than call the passed callbacks as-is.
//
// It can track two types of values - counts and time measurements, and
// got separate enums and separate functions for those.
//
// print() function outputs the tracked stats to stdout, using the supplied
// format strings. Counts need to be printed via %llu, and times are doubles
// - accumulated milliseconds for the operation.
//

class IncrementalStats {
public:
    enum class Action : int {
        TotalPages,
        SamePage,
        NotLoadedPage,
        StillZeroPage,
        SameHashPage,
        ChangedPage,
        ReusedPos,
        NewZeroPage,
        AppendedPos,
        /////////////////////
        Count
    };

    enum class Time : int {
        Disk,
        Hashing,
        ZeroCheck,
        GapTrackingWorker,
        GapTrackingWriter,
        Compressing,
        /////////////////////
        Count
    };

#if SNAPSHOT_PROFILE <= 1
    template <class Func>
    auto measure(Time time, Func&& func) -> decltype(func()) {
        return func();
    }

    void count(Action action) {}

    void print(const char* actionFormat,
               const char* timeFormat,
               const char* prefixFormat,
               ...) {}

#else   // SNAPSHOT_PROFILE > 1
    template <class Func>
    auto measure(Time time, Func&& func) -> decltype(func()) {
        return base::measure(mTimes[int(time)], std::forward<Func>(func));
    }

    void count(Action action) {
        mActions[int(action)].fetch_add(1, std::memory_order_relaxed);
    }

    void print(const char* actionFormat,
               const char* timeFormat,
               const char* prefixFormat,
               ...);

private:
    std::array<std::atomic<int64_t>, int(Action::Count)> mActions{};
    std::array<std::atomic<int64_t>, int(Time::Count)> mTimes{};
#endif  // SNAPSHOT_PROFILE > 1
};

}  // namespace snapshot
}  // namespace android
