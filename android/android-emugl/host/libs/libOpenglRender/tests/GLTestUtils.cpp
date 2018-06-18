// Copyright (C) 2018 The Android Open Source Project
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

#include "GLTestUtils.h"

namespace gltest {

testing::AssertionResult RowMatches(int rowIndex, size_t rowBytes,
                                    unsigned char* expected, unsigned char* actual) {
    for (size_t i = 0; i < rowBytes; ++i) {
        if (expected[i] != actual[i]) {
            return testing::AssertionFailure() <<
                       "row " << rowIndex << " byte " << i << " mismatch. expected: " <<
                       "(" << expected[i] << "), actual: " << "(" << actual[i] << ")";
        }
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult ImageMatches(int width, int height, int bpp, int rowLength,
                                      unsigned char* expected, unsigned char* actual) {
    int rowMismatches = 0;
    for (int i = 0; i < height * rowLength; i += rowLength) {
        size_t rowBytes = width * bpp;
        if (!RowMatches(i / rowLength, rowBytes, expected + i, actual + i)) {
            ++rowMismatches;
        }
    }

    if (rowMismatches) {
        return testing::AssertionFailure() <<
               "Mismatch in image: " << rowMismatches << " rows mismatch.";
    } else {
        return testing::AssertionSuccess();
    }
}

} // namespace gltest
