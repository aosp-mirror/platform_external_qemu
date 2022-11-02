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

#include "aemu/base/Stopwatch.h"

#include "android/base/system/System.h"

namespace android {
namespace base {

Stopwatch::Stopwatch() : mStartUs(System::get()->getHighResTimeUs()) {}

uint64_t Stopwatch::elapsedUs() const {
    return System::get()->getHighResTimeUs() - mStartUs;
}

uint64_t Stopwatch::restartUs() {
    const auto now = System::get()->getHighResTimeUs();
    const auto oldStart = mStartUs;
    mStartUs = now;
    return now - oldStart;
}

}  // namespace base
}  // namespace android
