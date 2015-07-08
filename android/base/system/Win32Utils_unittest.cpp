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

namespace android {
namespace base {

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

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

}  // namespace base
}  // namespace android
