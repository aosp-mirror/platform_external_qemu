// Copyright 2016 The Android Open Source Project
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

#pragma once

#include "android/base/TypeTraits.h"
#include <gtest/gtest.h>
#include <iterator>

// Miscellenaous helper declarations for unit-tests using the GoogleTest
// framework.

namespace android {
namespace base {

// RangesMatch is a useful template used to compare the content of two
// ranges at runtime. Usage is simply:
//
//   EXPECT_TRUE(RangesMatch(range1, range2);
//
// Where |range1| and |range2| must have the same item type, and size.
template <typename Range,
          typename = enable_if<is_range<Range>>>
inline ::testing::AssertionResult RangesMatch(const Range& expected,
                                              const Range& actual) {
    const auto expectedSize =
            std::distance(std::begin(expected), std::end(expected));
    const auto actualSize = std::distance(std::begin(actual), std::end(actual));
    if (actualSize != expectedSize) {
        return ::testing::AssertionFailure()
               << "actual range size " << actualSize << " != expected size "
               << expectedSize;
    }

    auto itExp = std::begin(expected);
    for (const auto& act : actual) {
        if (*itExp != act) {
            const auto index = std::distance(std::begin(expected), itExp);
            return ::testing::AssertionFailure()
                   << "range[" << index << "] (" << act << ") != expected["
                   << index << "] (" << *itExp << ")";
        }
        ++itExp;
    }

    return ::testing::AssertionSuccess();
}

}  // namespace base
}  // namespace android
