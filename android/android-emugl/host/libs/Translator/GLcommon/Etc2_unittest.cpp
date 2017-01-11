// Copyright 2015 The Android Open Source Project
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

#include <GLcommon/etc.h>

#include <gtest/gtest.h>
#include <stdio.h>

namespace {
class Etc2Test : public ::testing::Test {
protected:
    static const int cRgbPatchSize = 48;
    static const int cRgbEncodedSize = 8;
    static const int cAlphaPatchSize = 16;
    static const int cAlphaEncodedSize = 8;
    void decodeRgbTest(const etc1_byte* bytesEncoded, const etc1_byte* expectedDecoded) {
        etc1_byte decoded[cRgbPatchSize];
        etc2_decode_rgb_block(bytesEncoded, decoded);
        for (int i=0; i<cRgbPatchSize; i++) {
            EXPECT_EQ(expectedDecoded[i], decoded[i]);
        }
    }
    void decodeEacTest(const etc1_byte* bytesEncoded, int decodedElementBytes,
                       bool isSigned, const etc1_byte* expectedDecoded) {
        etc1_byte decoded[cAlphaPatchSize * decodedElementBytes];
        eac_decode_single_channel_block(bytesEncoded, decodedElementBytes, isSigned, decoded);
        for (int i=0; i<cAlphaPatchSize * decodedElementBytes; i++) {
            EXPECT_EQ(expectedDecoded[i], decoded[i]);
        }
    }
};
}

// ETC2 rgb decoder tests.
// The three test cases here cover T, H and planar codec, which are
// introduced in ETC2.

TEST_F(Etc2Test, ETC2T) {
    const unsigned char encoded[cRgbEncodedSize] = {21, 101, 186, 135, 166, 238, 74, 106};
    const unsigned char expectedDecoded[cRgbPatchSize] = {
        153, 102, 85,   153, 102, 85,   153, 102, 85,   153, 102, 85,
        171, 154, 120,  171, 154, 120,  171, 154, 120,  187, 170, 136,
        187, 170, 136,  171, 154, 120,  187, 170, 136,  203, 186, 152,
        171, 154, 120,  187, 170, 136,  203, 186, 152,  187, 170, 136};
    decodeRgbTest((const etc1_byte*)encoded, (const etc1_byte*)expectedDecoded);
}

TEST_F(Etc2Test, ETC2H) {
    const unsigned char encoded[cRgbEncodedSize] = {110, 13, 228, 186, 119, 119, 255, 117};
    const unsigned char expectedDecoded[cRgbPatchSize] = {
        198, 147, 113,  198, 147, 113,  198, 147, 113,    198, 147, 113,
        210, 159, 125,  198, 147, 113,  198, 147, 113,    198, 147, 113,
        198, 147, 113,  198, 147, 113,  198, 147, 113,    198, 147, 113,
        227, 210, 193,  227, 210, 193,  215, 198, 181,    215, 198, 181};
    decodeRgbTest((const etc1_byte*)encoded, (const etc1_byte*)expectedDecoded);
}

TEST_F(Etc2Test, ETC2P) {
    const unsigned char encoded[cRgbEncodedSize] = {89, 138, 250, 79, 120, 181, 146, 29};
    const unsigned char expectedDecoded[cRgbPatchSize] = {
        178, 139, 113,  173, 134, 107,  168, 130, 101,  163, 125, 95,
        178, 141, 114,  173, 136, 108,  168, 131, 102,  163, 126, 96,
        178, 142, 115,  173, 137, 109,  168, 133, 103,  163, 128, 97,
        178, 144, 116,  173, 139, 110,  168, 134, 104,  163, 129, 98};
    decodeRgbTest((const etc1_byte*)encoded, (const etc1_byte*)expectedDecoded);
}

// EAC alpha decoder tests.
TEST_F(Etc2Test, EAC_Alpha) {
    const unsigned char encoded[cAlphaEncodedSize]
                            = {101, 65, 229, 178, 147, 5, 32, 2};
    const unsigned char expectedDecoded[cAlphaPatchSize] = {
        149, 73, 89, 89,
        73, 61, 73, 89,
        49, 61, 61, 89,
        49, 49, 61, 61};
    decodeEacTest((const etc1_byte*)encoded, 1, false,
                  (const etc1_byte*)expectedDecoded);
}

// EAC 16bit decoder tests.
TEST_F(Etc2Test, DISABLED_EAC_R11_0) {
    const int elementBytes = 2;
    const unsigned char encoded[cAlphaEncodedSize]
                            = {0, 0, 36, 146, 73, 36, 146, 73};
    const unsigned char expectedDecoded[cAlphaPatchSize * elementBytes] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0};
    decodeEacTest((const etc1_byte*)encoded, elementBytes, false,
                  (const etc1_byte*)expectedDecoded);
}

TEST_F(Etc2Test, DISABLED_EAC_R11_NonZero) {
    const int elementBytes = 2;
    const unsigned char encoded[cAlphaEncodedSize]
                            = {45, 16, 23, 116, 38, 100, 86, 208};
    const unsigned char expectedDecoded[cAlphaPatchSize * elementBytes] = {
        133, 42, 132, 36, 131, 30, 131, 30,
        134, 50, 133, 42, 132, 39, 131, 30,
        134, 53, 133, 47, 133, 42, 132, 36,
        135, 59, 134, 53, 134, 50, 133, 42};
    decodeEacTest((const etc1_byte*)encoded, elementBytes, false,
                  (const etc1_byte*)expectedDecoded);
}

TEST_F(Etc2Test, DISABLED_EAC_SIGNED_R11) {
    const int elementBytes = 2;
    const unsigned char encoded[cAlphaEncodedSize]
                            = {198, 123, 74, 150, 120, 104, 6, 137};
    const unsigned char expectedDecoded[cAlphaPatchSize * elementBytes] = {
        230, 148, 1, 128, 1, 128, 1, 128,
        230, 148, 233, 162, 230, 148, 230, 148,
        249, 225, 1, 5, 238, 183, 233, 162,
        233, 162, 238, 183, 238, 183, 233, 162};
    decodeEacTest((const etc1_byte*)encoded, elementBytes, true,
                  (const etc1_byte*)expectedDecoded);
}

