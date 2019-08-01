// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/RateEstimator.h"

#include <gtest/gtest.h>

TEST(RateEstimator, Basic) {
    android::base::RateEstimator e("basicEvent", 10);
    e.addEvent();
    EXPECT_GE(e.getHz(), 0.0f);
    e.report();
}
