// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/network/MacAddress.h"

#include <gtest/gtest.h>

using android::network::MacAddress;

TEST(MacAddressTest, Basic) {
    const uint8_t kBroadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MacAddress broadcast(MACARG(kBroadcast));
    EXPECT_EQ(broadcast.isBroadcast(), true);

    const uint8_t kMac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    MacAddress addr(MACARG(kMac));
    EXPECT_EQ(addr.empty(), false);
    EXPECT_EQ(addr.isMulticast(), false);
    for (size_t i = 0; i < sizeof(kMac); i++)
        EXPECT_EQ(addr[i], kMac[i]);

    EXPECT_TRUE(broadcast != addr);
}