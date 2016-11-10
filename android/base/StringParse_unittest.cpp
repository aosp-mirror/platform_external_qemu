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

#include "android/base/StringParse.h"

#include "android/base/memory/ScopedPtr.h"
#include <gtest/gtest.h>

#include <locale.h>

namespace android {
namespace base {

TEST(StringFormat, SscanfWithCLocale) {
#ifdef _WIN32
    static const char commaLocale[] = "French";
#else
    static const char commaLocale[] = "fr_FR.UTF-8";
#endif

    // Set up a locale with comma as a decimal mark.
    auto oldLocale = setlocale(LC_ALL, nullptr);
    ASSERT_NE(nullptr, oldLocale);
    // setlocale() will overwrite the |oldLocale| pointer's data, so copy it.
    std::string oldLocaleCopy = oldLocale;
    auto newLocale = setlocale(LC_ALL, commaLocale);
    ASSERT_NE(nullptr, newLocale);
    ASSERT_STREQ(",", localeconv()->decimal_point);

    // Restore the locale after the test.
    const auto scopedLocaleRestorer = android::base::makeCustomScopedPtr(
            &oldLocaleCopy[0], [](const char* name) {
                ASSERT_NE(nullptr, setlocale(LC_ALL, name));
            });

    static const char comma[] = "1,3";
    static const char dot[] = "1.3";
    static const char format[] = "%f%n";
    float val = 0;
    int n = 0;

    // Now make sure we parse floating point strings as expected.
    EXPECT_EQ(1, sscanf(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1, val);
    EXPECT_EQ(1, n);
    EXPECT_EQ(1, sscanf(comma, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);

    // C-Locale parsing should be able to parse strings with C locale
    EXPECT_EQ(1, SscanfWithCLocale(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);

    // And the regular parsing still works as it used to.
    EXPECT_EQ(1, sscanf(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1, val);
    EXPECT_EQ(1, n);
    EXPECT_EQ(1, sscanf(comma, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);
}

}  // namespace base
}  // namespace android
