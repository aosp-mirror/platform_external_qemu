// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/bootconfig.h"

#include <string_view>
#include <gtest/gtest.h>
#include <string.h>

namespace android {

using namespace std::literals;

namespace {
constexpr std::string_view kBootconfigMagic = "#BOOTCONFIG\n"sv;

uint32_t loadLE32(const void* m) {
    const uint8_t* m8 = static_cast<const uint8_t*>(m);
    return uint32_t(m8[0]) | (uint32_t(m8[1]) << 8)
           | (uint32_t(m8[2]) << 16) | (uint32_t(m8[3]) << 24);
}
}  // namespace

TEST(buildBootconfigBlob, OptsMagic) {
    const std::vector<std::pair<std::string, std::string>> bootconfig = {
        {"a", "b"},
        {"c", "2"},
    };

    constexpr auto propsBlob = "a=\"b\"\nc=\"2\"\n\0"sv;

    const auto blob = buildBootconfigBlob(0, bootconfig);

    EXPECT_GT(blob.size(), propsBlob.size() + kBootconfigMagic.size());
    EXPECT_TRUE(!memcmp(blob.data(), propsBlob.data(), propsBlob.size()));
    EXPECT_TRUE(!memcmp(&blob[blob.size() - kBootconfigMagic.size()],
                        kBootconfigMagic.data(), kBootconfigMagic.size()));
}

TEST(buildBootconfigBlob, SizeAlignmentCsum) {
    const std::vector<std::pair<std::string, std::string>> bootconfig = {
        {"a", "b"},
    };

    constexpr auto propsBlob = "a=\"b\"\n\0"sv;  // 7 byte long
    constexpr uint32_t propsCsum = 'a' + '=' + '\"' + 'b' + '\"' + '\n';

    {
        const auto blob = buildBootconfigBlob(0, bootconfig);
        EXPECT_EQ(blob.size() - kBootconfigMagic.size() - 8 - propsBlob.size(), 1);

        const char* lencsum = &blob[blob.size() - kBootconfigMagic.size() - 8];
        EXPECT_EQ(loadLE32(lencsum), propsBlob.size() + 1);
        EXPECT_EQ(loadLE32(lencsum + 4), propsCsum + '+');
    }
    {
        const auto blob = buildBootconfigBlob(1, bootconfig);
        EXPECT_EQ(blob.size() - kBootconfigMagic.size() - 8 - propsBlob.size(), 0);

        const char* lencsum = &blob[blob.size() - kBootconfigMagic.size() - 8];
        EXPECT_EQ(loadLE32(lencsum), propsBlob.size());
        EXPECT_EQ(loadLE32(lencsum + 4), propsCsum);
    }
    {
        const auto blob = buildBootconfigBlob(2, bootconfig);
        EXPECT_EQ(blob.size() - kBootconfigMagic.size() - 8 - propsBlob.size(), 3);

        const char* lencsum = &blob[blob.size() - kBootconfigMagic.size() - 8];
        EXPECT_EQ(loadLE32(lencsum), propsBlob.size() + 3);
        EXPECT_EQ(loadLE32(lencsum + 4), propsCsum + '+' + '+' + '+');
    }
    {
        const auto blob = buildBootconfigBlob(3, bootconfig);
        EXPECT_EQ(blob.size() - kBootconfigMagic.size() - 8 - propsBlob.size(), 2);

        const char* lencsum = &blob[blob.size() - kBootconfigMagic.size() - 8];
        EXPECT_EQ(loadLE32(lencsum), propsBlob.size() + 2);
        EXPECT_EQ(loadLE32(lencsum + 4), propsCsum + '+' + '+');
    }
}

}  // namespace android
