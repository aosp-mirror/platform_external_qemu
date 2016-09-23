// Copyright 2016 The Android Open Source Project
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

#include "android/base/async/GenLooper.h"

namespace android {
namespace base {

// A TestLooper is a Looper implementation that allows its user to inspect all
// internals.
//
class TestLooper : public GenLooper {
public:
    using GenLooper::GenLooper;

    // Add some functions to uncover the internal state
    const TimerSet& timers() const;
    const TimerList& activeTimers() const;
    const TimerList& pendingTimers() const;

    const FdWatchSet& fdWatches() const;
    const FdWatchList& pendingFdWatches() const;

    bool exitRequested() const;

    using GenLooper::runOneIterationWithDeadlineMs;
};

const GenLooper::TimerSet& TestLooper::timers() const {
    return mTimers;
}

const GenLooper::TimerList& TestLooper::activeTimers() const {
    return mActiveTimers;
}

const GenLooper::TimerList& TestLooper::pendingTimers() const {
    return mPendingTimers;
}

const GenLooper::FdWatchSet& TestLooper::fdWatches() const {
    return mFdWatches;
}

const GenLooper::FdWatchList& TestLooper::pendingFdWatches() const {
    return mPendingFdWatches;
}

bool TestLooper::exitRequested() const {
    return mForcedExit;
}

}  // namespace base
}  // namespace android
