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
// format string and arguments to format the prefix for the information.
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

    static constexpr char kActionFormat[] =
            "\tPages: total %llu\n"
            "\t\tsame %llu [not loaded %llu; still empty %llu; "
            "same hash %llu]\n"
            "\t\tnew  %llu [reused %llu, empty %llu, appended %llu]\n";

    enum class Time : int {
        Hashing,
        ZeroCheck,
        GapTrackingWorker,
        GapTrackingWriter,
        Compressing,
        WaitingForDisk,
        TotalHandlingPageSave,
        DiskWriteCombine,
        DiskIndexWrite,
        /////////////////////
        Count
    };

    static constexpr char kTimeFormat[] =
            "\tTimes: hash %.03f, iszero %.03f, gaps %.03f/%.03f, "
            "lz4 %.03f, waitdisk %.03f, totalHandlingPageSave %.03f, "
            "diskWriteCombine %.03f, diskIndexWrite %.03f\n";

#if SNAPSHOT_PROFILE <= 1
    template <class Func>
    auto measure(Time time, Func&& func) -> decltype(func()) {
        return func();
    }

    void count(Action action) {}
    void countMultiple(Action action, int64_t howMany) {}

    void print(const char* prefixFormat, ...) {}

#else   // SNAPSHOT_PROFILE > 1
    template <class Func>
    auto measure(Time time, Func&& func) -> decltype(func()) {
        return base::measure(mTimes[int(time)], std::forward<Func>(func));
    }

    void count(Action action) {
        countMultiple(action, 1);
    }

    void countMultiple(Action action, int64_t howMany) {
        mActions[int(action)].fetch_add(howMany, std::memory_order_relaxed);
    }

    void print(const char* prefixFormat, ...);

private:
    std::array<std::atomic<int64_t>, int(Action::Count)> mActions{};
    std::array<std::atomic<int64_t>, int(Time::Count)> mTimes{};
#endif  // SNAPSHOT_PROFILE > 1
};

}  // namespace snapshot
}  // namespace android
