// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/misc/StringUtils.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// Tiny random string generator
struct StringGenerator {
    StringGenerator() : mSeed(123) {}

    String next() {
        uint32_t seed = mSeed;
        mSeed = mSeed * 5137 + 143;

        String result;
        uint32_t len = (seed % 3) + 2;
        seed /= 3;
        for (uint32_t n = 0; n < len; ++n) {
            uint32_t x = (seed % 10);
            seed /= 10;
            static const char* kFragments[10] = {
                "bo",
                "han",
                "xun",
                "li",
                "me",
                "ton",
                "zy",
                "la",
                "mo",
                "tar"
            };
            result.append(kFragments[x]);
        }
        return result;
    }

    uint32_t mSeed;
};

// Checks that all items in |a| appear also in |b|.
bool checkStringVectorContains(const StringVector& a,
                               const StringVector& b) {
    for (size_t n = 0; n < a.size(); ++n) {
        const String& sa = a[n];
        bool found = false;
        for (size_t m = 0; m < b.size(); ++m) {
            if (sa == b[m]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(StringUtils, sortStringArray) {
    StringGenerator generator;
    StringVector strings;
    StringVector sorted;

    const size_t kCount = 100;
    for (size_t n = 0; n < kCount; ++n) {
        String s = generator.next();
        strings.append(s);
        sorted.append(s);
    }

    sortStringArray(&sorted[0], sorted.size());

    // Check that the array is sorted.
    for (size_t n = 1; n < kCount; ++n) {
        EXPECT_LE(::strcmp(sorted[n - 1].c_str(), sorted[n].c_str()), 0) << n;
    }

    // Check that all items in sorted are
    EXPECT_TRUE(checkStringVectorContains(strings, sorted));
    EXPECT_TRUE(checkStringVectorContains(sorted, strings));
}

}  // namespace base
}  // namespace android
