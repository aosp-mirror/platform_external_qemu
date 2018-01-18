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

#include "android/snapshot/IncrementalStats.h"

#include <stdarg.h>

namespace android {
namespace snapshot {

constexpr char IncrementalStats::kActionFormat[];
constexpr char IncrementalStats::kTimeFormat[];

#if SNAPSHOT_PROFILE > 1

template <class TransformFunc, size_t N, int... Idx>
static void formatFromArrayImpl(const char* format,
                                std::array<std::atomic<int64_t>, N>& arr,
                                TransformFunc&& transform,
                                base::Seq<Idx...>) {
    printf(format, transform(arr[Idx].load(std::memory_order_relaxed))...);
}

template <class TransformFunc, size_t N>
static void formatFromArray(const char* format,
                            std::array<std::atomic<int64_t>, N>& arr,
                            TransformFunc&& transform) {
    return formatFromArrayImpl(format, arr,
                               std::forward<TransformFunc>(transform),
                               base::MakeSeqT<N>{});
}

void IncrementalStats::print(const char* prefixFormat, ...) {
    va_list args;
    va_start(args, prefixFormat);
    vprintf(prefixFormat, args);
    va_end(args);

    formatFromArray(kActionFormat, mActions,
                    [](int64_t x) { return (unsigned long long)x; });
    formatFromArray(kTimeFormat, mTimes, [](int64_t x) { return x / 1000.0; });
}

#endif  // SNAPSHOT_PROFILE > 1
}  // namespace snapshot
}  // namespace android
