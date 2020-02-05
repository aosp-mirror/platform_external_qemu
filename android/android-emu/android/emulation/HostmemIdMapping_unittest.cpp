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

#include "android/emulation/HostmemIdMapping.h"

#include <gtest/gtest.h>

using android::emulation::HostmemIdMapping;

// Tests creation and destruction.
TEST(HostmemIdMapping, Basic) {
    HostmemIdMapping m;
}

// Tests basic operations on an entry: add, remove, get entry info
TEST(HostmemIdMapping, BasicEntry) {
    HostmemIdMapping m;
    auto id = m.add(0, 1);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, id);
    id = m.add(1, 0);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, id);
    id = m.add(1, 2);
    EXPECT_NE(HostmemIdMapping::kInvalidHostmemId, id);

    auto entry = m.get(id);
    EXPECT_EQ(id, entry.id);
    EXPECT_EQ(1, entry.hva);
    EXPECT_EQ(2, entry.size);

    m.remove(id);

    entry = m.get(id);

    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);
}

// Tests the clear() method.
TEST(HostmemIdMapping, Clear) {
    HostmemIdMapping m;
    auto id1 = m.add(1, 2);
    auto id2 = m.add(3, 4);

    m.clear();

    auto entry = m.get(id1);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);

    entry = m.get(id2);
    EXPECT_EQ(HostmemIdMapping::kInvalidHostmemId, entry.id);
    EXPECT_EQ(0, entry.hva);
    EXPECT_EQ(0, entry.size);
}

