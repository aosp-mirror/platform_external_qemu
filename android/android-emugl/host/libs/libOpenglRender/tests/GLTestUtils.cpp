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

using android::AlignedBuf;

namespace emugl {

testing::AssertionResult RowMatches(int rowIndex, size_t rowBytes,
                                    unsigned char* expected, unsigned char* actual) {
    for (size_t i = 0; i < rowBytes; ++i) {
        if (expected[i] != actual[i]) {
            return testing::AssertionFailure()
                   << "row " << rowIndex << " byte " << i
                   << " mismatch. expected: "
                   << "(" << testing::PrintToString(expected[i])
                   << "), actual: "
                   << "(" << testing::PrintToString(actual[i]) << ")";
        }
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult ImageMatches(int width, int height, int bpp, int rowLength,
                                      unsigned char* expected, unsigned char* actual) {
    int rowMismatches = 0;
    std::stringstream rowList;
    std::stringstream rowDetails;
    rowList << "on ";
    for (int i = 0; i < height * rowLength; i += rowLength) {
        size_t rowBytes = width * bpp;
        testing::AssertionResult rowResult = RowMatches(
                i / rowLength, rowBytes, expected + i * bpp, actual + i * bpp);
        if (!rowResult) {
            ++rowMismatches;
            if (rowMismatches < 50) {
                rowList << ((rowMismatches != 1) ? ", " : "")
                        << (i / rowLength);
                rowDetails << "\t" << rowResult.message() << "\n";
            }
            if (rowMismatches == 50) {
                rowList << " and more";
                rowDetails << "\t...";
            }
        }
    }
    if (rowMismatches) {
        return testing::AssertionFailure()
               << "Mismatch in image: " << rowMismatches << " rows mismatch "
               << rowList.str() << ".\n"
               << rowDetails.str();
    } else {
        return testing::AssertionSuccess();
    }
}

TestTexture createTestPatternRGB888(int width, int height) {
    int bpp = 3;

    TestTexture res(bpp * width * height);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            res[i * bpp * width + j * bpp + 0] = i % 0x100;
            res[i * bpp * width + j * bpp + 1] = j % 0x100;
            res[i * bpp * width + j * bpp + 2] = (i * width + j) % 0x100;
        }
    }

    return res;
}

TestTexture createTestPatternRGBA8888(int width, int height) {
    int bpp = 4;

    TestTexture res(bpp * width * height);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            res[i * bpp * width + j * bpp + 0] = i % 0x100;
            res[i * bpp * width + j * bpp + 1] = j % 0x100;
            res[i * bpp * width + j * bpp + 2] = (i * width + j) % 0x100;
            res[i * bpp * width + j * bpp + 3] = (width - j) % 0x100;
        }
    }

    return res;
}

static float clamp(float x, float low, float high) {
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

TestTexture createTestTextureRGB888SingleColor(int width, int height, float r, float g, float b) {
    int bpp = 3;

    TestTexture res(bpp * width * height);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            res[i * bpp * width + j * bpp + 0] = clamp(r, 0.0f, 1.0f) * 255.0f;
            res[i * bpp * width + j * bpp + 1] = clamp(g, 0.0f, 1.0f) * 255.0f;
            res[i * bpp * width + j * bpp + 2] = clamp(b, 0.0f, 1.0f) * 255.0f;
        }
    }

    return res;
}

TestTexture createTestTextureRGBA8888SingleColor(int width, int height, float r, float g, float b, float a) {
    int bpp = 4;

    TestTexture res(bpp * width * height);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            res[i * bpp * width + j * bpp + 0] = clamp(r, 0.0f, 1.0f) * 255.0f;
            res[i * bpp * width + j * bpp + 1] = clamp(g, 0.0f, 1.0f) * 255.0f;
            res[i * bpp * width + j * bpp + 2] = clamp(b, 0.0f, 1.0f) * 255.0f;
            res[i * bpp * width + j * bpp + 3] = clamp(a, 0.0f, 1.0f) * 255.0f;
        }
    }

    return res;
}

} // namespace emugl
