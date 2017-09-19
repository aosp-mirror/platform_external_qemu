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

    static double sec(System::WallDuration us) {
        return us / 1000000.0;
    }

private:
    System::WallDuration mStartUs;
};

#define STOPWATCH_PRINT_SPLIT(sw) \
    fprintf(stderr, "%s:%d %.03f ms\n", __func__, __LINE__, (sw).restartUs() / 1000.0f);

#define STOPWATCH_PRINT(sw) \
    fprintf(stderr, "%s:%d %.03f ms total\n", __func__, __LINE__, (sw).elapsedUs() / 1000.0f);

}  // namespace base
}  // namespace android
