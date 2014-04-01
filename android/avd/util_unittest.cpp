// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/avd/util.h"

#include <gtest/gtest.h>

TEST(AvdUtil, emulator_getBackendSuffix) {
  EXPECT_STREQ("arm", emulator_getBackendSuffix("arm"));
  EXPECT_STREQ("x86", emulator_getBackendSuffix("x86"));
  EXPECT_STREQ("x86", emulator_getBackendSuffix("x86_64"));
  EXPECT_STREQ("mips", emulator_getBackendSuffix("mips"));

  // TODO(digit): Add support for these CPU architectures to the emulator
  // to change these to EXPECT_STREQ() calls.
  EXPECT_FALSE(emulator_getBackendSuffix("arm64"));
  EXPECT_FALSE(emulator_getBackendSuffix("mips64"));

  EXPECT_FALSE(emulator_getBackendSuffix(NULL));
  EXPECT_FALSE(emulator_getBackendSuffix("dummy"));
}
