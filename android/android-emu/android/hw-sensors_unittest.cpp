// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <gtest/gtest.h>

#include "android/hw-sensors.h"
#include "android/globals.h"

TEST(Hw_sensors, android_foldable_initialize) {
    static const struct FoldableState* ret;

    android_hw->hw_sensor_hinge =  true;
    android_hw->hw_sensor_hinge_count = 2;
    android_hw->hw_sensor_hinge_type = 0;
    android_hw->hw_sensor_hinge_ranges = (char*)"45- 360, 0-180";
    android_hw->hw_sensor_hinge_defaults = (char*)"180,90";
    ret = android_foldable_initialize(nullptr);

    EXPECT_EQ(180, ret->currentHingeDegrees[0]);
    EXPECT_EQ(90, ret->currentHingeDegrees[1]);
    EXPECT_EQ(2, ret->config.numHinges);
    EXPECT_EQ(0, ret->config.displayId);
    EXPECT_EQ(ANDROID_FOLDABLE_HORIZONTAL_SPLIT, ret->config.hingesType);
    EXPECT_EQ(45, ret->config.hingeParams[0].minDegrees);
    EXPECT_EQ(360, ret->config.hingeParams[0].maxDegrees);
    EXPECT_EQ(0, ret->config.hingeParams[1].minDegrees);
    EXPECT_EQ(180, ret->config.hingeParams[1].maxDegrees);
    EXPECT_EQ(180, ret->config.hingeParams[0].defaultDegrees);
    EXPECT_EQ(90, ret->config.hingeParams[1].defaultDegrees);
}