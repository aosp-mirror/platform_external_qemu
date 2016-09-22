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

#include "android/base/async/DefaultLooper.h"

namespace android {
namespace base {

// A TestLooper is a Looper implementation that allows its user to inspect all
// internals.
//
class TestLooper : public DefaultLooper {
public:
    using DefaultLooper::DefaultLooper;

    // Add some functions to uncover the internal state
    const TimerSet& timers() const;
    const TimerList& activeTimers() const;
    const TimerList& pendingTimers() const;

    const FdWatchSet& fdWatches() const;
    const FdWatchList& pendingFdWatches() const;

    bool exitRequested() const;
};

const DefaultLooper::TimerSet& TestLooper::timers() const {
    return mTimers;
}

const DefaultLooper::TimerList& TestLooper::activeTimers() const {
    return mActiveTimers;
}

const DefaultLooper::TimerList& TestLooper::pendingTimers() const {
    return mPendingTimers;
}

const DefaultLooper::FdWatchSet& TestLooper::fdWatches() const {
    return mFdWatches;
}

const DefaultLooper::FdWatchList& TestLooper::pendingFdWatches() const {
    return mPendingFdWatches;
}

bool TestLooper::exitRequested() const {
    return mForcedExit;
}

}  // namespace base
}  // namespace android
