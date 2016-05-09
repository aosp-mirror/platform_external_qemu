// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/Base64.h"

#include <gtest/gtest.h>

namespace android {

TEST(Base64, base64Encode) {
    static const struct {
        const char* expected;
        const char* input;
    } kData[] = {
            {"", ""},
            {"Lw==", "/"},
            {"YWI=", "ab"},
            {"AVhh", "\x01Xa"},
            {"AVhhYg==", "\x01Xab"},
            {"AlhhYmM=", "\x02Xabc"},
            {"A1hhYmNk", "\x03Xabcd"},
            {"BFhhYmNkZQ==", "\x04Xabcde"},
            {"BVhhYmNkZWY=", "\x05Xabcdef"},
            {"d1hhYmNkZWZn", "\x77Xabcdefg"},
            {"UHJldHR5IGZseSBmb3IgYSByYWJiaQ==", "Pretty fly for a rabbi"},
    };
    for (const auto& entry : kData) {
        EXPECT_STREQ(entry.expected, base64Encode(entry.input).c_str())
                << "For input [" << entry.input << "]";
    }
}

TEST(Base64, base64EncodeSizeForInputSize) {
    static const struct {
        size_t expected;
        size_t input;
    } kData[] = {
        { 0, 0 },
        { 4, 1 },
        { 4, 2, },
        { 4, 3 },
        { 8, 4 },
        { 8, 5 },
        { 8, 6 },
        { 12, 7 },
        { 16, 12 },
        { 336, 252 },
    };
    for (const auto& entry : kData) {
        EXPECT_EQ(entry.expected, base64EncodedSizeForInputSize(entry.input))
            << "For input size " << entry.input;
    }
}

TEST(Base64, base64Decode) {
    static const struct {
        const char* input;
        const char* expected;
        bool result;
    } kData[] = {
            {"", "", true},
            {"Lw==", "/", true},
            {"YWI=", "ab", true},
            {"AVhh", "\x01Xa", true},
            {"AVhhYg==", "\x01Xab", true},
            {"AlhhYmM=", "\x02Xabc", true},
            {"A1hhYmNk", "\x03Xabcd", true},
            {"BFhhYmNkZQ==", "\x04Xabcde", true},
            {"BVhhYmNkZWY=", "\x05Xabcdef", true},
            {"d1hhYmNkZWZn", "\x77Xabcdefg", true},
            {"UHJldHR5IGZseSBmb3IgYSByYWJiaQ==", "Pretty fly for a rabbi",
             true},
            {".", nullptr, false},
            {"A", nullptr, false},
            {"=", nullptr, false},
            {"A=", nullptr, false},
            {"A==", nullptr, false},
            {"A===", nullptr, false},
            {"BA", "\x04", true},
    };
    for (const auto& entry : kData) {
        auto decoded = base64Decode(entry.input);
        EXPECT_EQ(entry.result, !!decoded)
                << "For input [" << entry.input << "]";
        if (entry.result) {
            EXPECT_STREQ(entry.expected, decoded->c_str()) << "For input ["
                                                           << entry.input << "]";
        }
    }
}

}  // namespace android
