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

#include "android/physics/BodyModel.h"

#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <assert.h>

using android::base::TestSystem;
using android::base::System;
using android::physics::BodyModel;

TEST(BodyModel, DefaultParameters) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);

    BodyModel bodyModel;

    constexpr ParameterValueType valueTypes[] = {
            PARAMETER_VALUE_TYPE_TARGET, PARAMETER_VALUE_TYPE_CURRENT,
            PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION,
            PARAMETER_VALUE_TYPE_DEFAULT};
    for (auto valueType : valueTypes) {
        SCOPED_TRACE(testing::Message() << "valueType=" << valueType);

        EXPECT_EQ(0.f, bodyModel.getHeartRate(valueType));
    }
}

TEST(BodyModel, SetHeartRate) {
    TestSystem mTestSystem("/", System::kProgramBitness);
    mTestSystem.setLiveUnixTime(false);
    mTestSystem.setUnixTime(1);
    BodyModel bodyModel;
    bodyModel.setHeartRate(100.f, PHYSICAL_INTERPOLATION_STEP);
    EXPECT_EQ(100.f, bodyModel.getHeartRate());
}
