// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/location/MapsKey.h"

#include <gtest/gtest.h>

namespace android_location {

TEST(MapsKey, InitialState) {
    auto m = android::location::MapsKey::get();
    EXPECT_STREQ(m->userMapsKey(), "");
    EXPECT_STREQ(m->androidStudioMapsKey(), "");
}

TEST(MapsKey, ReadWriteUserKey) {
    const char* key = "abcde";
    auto m = android::location::MapsKey::get();
    m->setUserMapsKey(key);

    auto m2 = android::location::MapsKey::get();
    EXPECT_STREQ(m->userMapsKey(),
                 key);
}

TEST(MapsKey, ReadWriteAndroidStudioKey) {
    const char* key = "abcde";
    auto m = android::location::MapsKey::get();
    m->setAndroidStudioMapsKey(key);

    auto m2 = android::location::MapsKey::get();
    EXPECT_STREQ(m->androidStudioMapsKey(),
                 key);
}

}  // namespace android_location
