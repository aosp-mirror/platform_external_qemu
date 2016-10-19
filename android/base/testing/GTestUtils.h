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

#include <gtest/gtest.h>

// Miscellenaous helper declarations for unit-tests using the GoogleTest
// framework.

// ArraysMatch is a useful template used to compare the content of two
// static arrays at runtime. Usage is simply:
//
//   EXPECT_TRUE(ArraysMatch(array1, array2);
//
// Where |array1| and |array2| must have the same item type, and size.
template <typename T, size_t size>
inline ::testing::AssertionResult ArraysMatch(const T (&expected)[size],
                                              const T (&actual)[size]) {
    for (size_t i(0); i < size; ++i) {
        if (expected[i] != actual[i]) {
            return ::testing::AssertionFailure()
                   << "array[" << i << "] (" << actual[i] << ") != expected["
                   << i << "] (" << expected[i] << ")";
        }
    }

    return ::testing::AssertionSuccess();
}
