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

#include "android/base/IOVector.h"
#include "android/base/ArraySize.h"

#include <gtest/gtest.h>
#include <vector>

using android::base::IOVector;

TEST(IOVectorTest, Basic) {
    IOVector iovector;
    EXPECT_EQ(iovector.size(), 0);
    const size_t kSize = 10;
    std::vector<uint8_t> buf(kSize);
    const struct iovec iov = {.iov_base = buf.data(), .iov_len = buf.size()};
    iovector.push_back(iov);
    EXPECT_EQ(iovector.size(), 1);
    EXPECT_EQ(iovector[0].iov_base, buf.data());
    EXPECT_EQ(iovector[0].iov_len, buf.size());
    EXPECT_EQ(iovector.summedLength(), buf.size());
    struct iovec vecs[] = {iov, iov};
    IOVector iovector2(vecs, vecs + android::base::arraySize(vecs));
    EXPECT_EQ(iovector2.size(), android::base::arraySize(vecs));
}

TEST(IOVectorTest, Copy) {
    IOVector vec;
    const size_t kSize = 100;
    std::vector<uint8_t> source(kSize, 1);
    std::vector<uint8_t> destination(kSize, 0);
    EXPECT_NE(memcmp(source.data(), destination.data(), kSize), 0);
    // Test copyTo
    vec.push_back({.iov_base = source.data(), .iov_len = source.size()});
    vec.copyTo(destination.data(), 0, kSize);
    EXPECT_EQ(memcmp(source.data(), destination.data(), kSize), 0);

    vec.clear();
    destination.clear();
    destination.insert(destination.end(), kSize, 0);
    // Test Copy From
    vec.push_back(
            {.iov_base = destination.data(), .iov_len = destination.size()});
    EXPECT_EQ(memcmp(vec[0].iov_base, destination.data(), vec.summedLength()),
              0);
    int copyOffset = 50;
    vec.copyFrom(source.data(), copyOffset, source.size() - copyOffset);
    EXPECT_EQ(memcmp(&(static_cast<uint8_t*>(vec[0].iov_base)[copyOffset]),
                     source.data() + copyOffset, source.size() - copyOffset),
              0);

    EXPECT_EQ(memcmp(vec[0].iov_base, destination.data(), copyOffset), 0);

    // Test AppendEntries to
    IOVector dest;
    EXPECT_NE(dest.summedLength(), vec.summedLength());
    vec.appendEntriesTo(&dest, 0, vec.summedLength());
    EXPECT_EQ(dest.summedLength(), vec.summedLength());
    EXPECT_EQ(dest.size(), vec.size());
    EXPECT_EQ(vec[0].iov_base, dest[0].iov_base);
    EXPECT_EQ(vec[0].iov_len, dest[0].iov_len);
}
