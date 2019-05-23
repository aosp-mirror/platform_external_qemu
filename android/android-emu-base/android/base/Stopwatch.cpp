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

#include "android/base/Stopwatch.h"

namespace android {
namespace base {

Stopwatch::Stopwatch() : mStartUs(System::get()->getHighResTimeUs()) {}

System::WallDuration Stopwatch::elapsedUs() const {
    return System::get()->getHighResTimeUs() - mStartUs;
}

System::WallDuration Stopwatch::restartUs() {
    const auto now = System::get()->getHighResTimeUs();
    const auto oldStart = mStartUs;
    mStartUs = now;
    return now - oldStart;
}

}  // namespace base
}  // namespace android
