// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/Utf8Utils.h"

#include <gtest/gtest.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

#define DATA(expected, text) { expected, text, sizeof(text) - 1U }

TEST(Utf8Utils, utf8IsValid) {
    static const struct {
        bool expected;
        const char* text;
        size_t textLen;
    } kData[] = {
        DATA(true, ""),
        DATA(true, "Hello World!"),
        DATA(true, "T\xC3\xA9l\xC3\xA9vision"),
        DATA(false, "\x80\x80"),
        DATA(true, "\xC1\xB6"),
        DATA(false, "\xC0\x7f"),
        DATA(true, "foo\xE0\x80\x80"),
        DATA(false, "foo\xE0\x80"),
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataSize; ++n) {
        EXPECT_EQ(kData[n].expected,
                  utf8IsValid(kData[n].text,
                              kData[n].textLen)) << "#" << n;
    }
}

}  // namespace base
}  // namespace android
