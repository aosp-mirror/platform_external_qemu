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

#include "android/base/system/Win32UnicodeString.h"

#include <gtest/gtest.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

namespace android {
namespace base {

TEST(Win32UnicodeString, DefaultConstructor) {
    Win32UnicodeString str;
    EXPECT_EQ(0u, str.size());
    EXPECT_STREQ(L"", str.c_str());
    EXPECT_TRUE(str.data());
}

TEST(Win32UnicodeString, Constructors) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
            {"", L""},
            {"Hello World!", L"Hello World!"},
            {"T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision"},
            {"foo\xE1\x80\x80 bar", L"foo\x1000 bar"},
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        Win32UnicodeString str1(kData[n].utf8);
        EXPECT_EQ(wcslen(kData[n].utf16), str1.size());
        EXPECT_STREQ(kData[n].utf16, str1.c_str());

        Win32UnicodeString str2(kData[n].utf8, strlen(kData[n].utf8));
        EXPECT_EQ(wcslen(kData[n].utf16), str2.size());
        EXPECT_STREQ(kData[n].utf16, str2.c_str());

        String baseStr(kData[n].utf8);
        Win32UnicodeString str3(baseStr);
        EXPECT_EQ(wcslen(kData[n].utf16), str3.size());
        EXPECT_STREQ(kData[n].utf16, str3.c_str());

        size_t utf16Len = wcslen(kData[n].utf16);
        Win32UnicodeString str4(kData[n].utf16);
        EXPECT_EQ(utf16Len, str4.size());
        EXPECT_STREQ(kData[n].utf16, str4.c_str());
    }
}

TEST(Win32UnicodeString, convertToUtf8) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
            {"", L""},
            {"Hello World!", L"Hello World!"},
            {"T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision"},
            {"foo\xE1\x80\x80 bar", L"foo\x1000 bar"},
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        String str1 = Win32UnicodeString::convertToUtf8(kData[n].utf16);
        EXPECT_EQ(strlen(kData[n].utf8), str1.size());
        EXPECT_STREQ(kData[n].utf8, str1.c_str());

        String str2 = Win32UnicodeString::convertToUtf8(kData[n].utf16,
                                                        wcslen(kData[n].utf16));
        EXPECT_EQ(strlen(kData[n].utf8), str2.size());
        EXPECT_STREQ(kData[n].utf8, str2.c_str());
    }
}

}  // namespace base
}  // namespace android
