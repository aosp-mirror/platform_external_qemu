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

#include "android/base/system/Win32Utils.h"

#include <gtest/gtest.h>

#include <wchar.h>

namespace android {
namespace base {

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

typedef Win32Utils::UnicodeString UnicodeString;

TEST(Win32Utils, quoteCommandLine) {
  static const struct {
      const char* input;
      const char* expected;
  } kData[] = {
      { "foo", "foo" },
      { "foo bar", "\"foo bar\"" },
      { "foo\\bar", "foo\\bar" },
      { "foo\\\\bar", "foo\\\\bar" },
      { "foo\"bar", "\"foo\\\"bar\"" },
      { "foo\\\"bar", "\"foo\\\\\\\"bar\"" },
      { "foo\\bar zoo", "\"foo\\bar zoo\"" },
  };
  for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
      const char* input = kData[n].input;
      const char* expected = kData[n].expected;

      String out = Win32Utils::quoteCommandLine(input);
      EXPECT_STREQ(expected, out.c_str()) << "Quoting '" << input << "'";
  }
}

TEST(Win32Utils, UnicodeStringDefaultConstructor) {
    UnicodeString str;
    EXPECT_EQ(0u, str.size());
    EXPECT_STREQ(L"", str.c_str());
    EXPECT_EQ(nullptr, str.data());
}

TEST(Win32Utils, UnicodeStringConstructors) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
        { "", L"" },
        { "Hello World!", L"Hello World!" },
        { "T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision" },
        { "foo\xE1\x80\x80 bar", L"foo\x1000 bar" },
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        UnicodeString str1(kData[n].utf8);
        EXPECT_EQ(wcslen(kData[n].utf16), str1.size());
        EXPECT_STREQ(kData[n].utf16, str1.c_str());

        UnicodeString str2(kData[n].utf8, strlen(kData[n].utf8));
        EXPECT_EQ(wcslen(kData[n].utf16), str2.size());
        EXPECT_STREQ(kData[n].utf16, str2.c_str());

        String baseStr(kData[n].utf8);
        UnicodeString str3(baseStr);
        EXPECT_EQ(wcslen(kData[n].utf16), str3.size());
        EXPECT_STREQ(kData[n].utf16, str3.c_str());

        size_t utf16Len = wcslen(kData[n].utf16);
        UnicodeString str4(kData[n].utf16);
        EXPECT_EQ(utf16Len, str4.size());
        EXPECT_STREQ(kData[n].utf16, str4.c_str());
    }
}

}  // namespace base
}  // namespace android
