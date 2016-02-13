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

#include "android/base/files/FilePath.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

#ifdef _WIN32
    const FilePath sAbsolute(PATH_LITERAL("C:\\temp"));
#else
    const FilePath sAbsolute(PATH_LITERAL("/tmp"));
#endif

TEST(FilePath, construction) {
    FilePath path(PATH_LITERAL("some path"));
    FilePath::StringType str(PATH_LITERAL("another path"));
    FilePath anotherPath(str);

#ifdef _WIN32
    FilePath pathFromUtf8("this is a plain string");
    FilePath pathFromStdString(std::string("this is an std::string"));
#endif
}

TEST(FilePath, native) {
#ifdef _WIN32
    std::wstring native = L"C:\\Temp";
#else
    std::string native = "/tmp";
#endif
    FilePath path(native);

    // A copy should have been made, no references or pointers
    ASSERT_NE(&native, &path.native());
    // But the content should have been preserved
    ASSERT_STREQ(native.c_str(), path.native().c_str());
}

TEST(FilePath, c_str) {
#ifdef _WIN32
    std::wstring native = L"C:\\Temp";
#else
    std::string native = "/tmp";
#endif
    FilePath path(native);

    ASSERT_NE(nullptr, path.c_str());
    ASSERT_STREQ(native.c_str(), path.c_str());
}

TEST(FilePath, assignAndCopy) {
    FilePath first(PATH_LITERAL("path of glory"));
    FilePath second(PATH_LITERAL("path of cows"));

    FilePath third(first);
    ASSERT_NE(&first.native(), &third.native());
    ASSERT_STREQ(first.c_str(), third.c_str());

    FilePath& result = third = second;
    ASSERT_NE(&second.native(), &third.native());
    ASSERT_STREQ(second.c_str(), third.c_str());
    // Should have returned reference to self
    ASSERT_EQ(&result, &third);
}

TEST(FilePath, isAbsolute) {
#ifdef _WIN32
    std::wstring absolutePaths[] = { L"C:\\",
                                     L"X:\\",
                                     L"D:\\temp",
                                     L"F:\\temp\\dir\\",
                                     L"\\\\localhost\\c\\temp" // UNC path
                                   };
    // A note here about "C:", this is a relative path and "copy C:file.txt ."
    // will copy file.txt from whatever the current working directory is on C:
    std::wstring relativePaths[] = { L"C:",
                                     L"/",
                                     L"\\",
                                     L"/tmp",
                                     L"foo\\bar\\baz\\",
                                     L"foo",
                                     L"\\foo",
                                     L""
                                   };
#else
    std::string absolutePaths[] = { "/",
                                    "//",
                                    "///",
                                    "/foo",
                                    "/foo/bar/baz/",
                                  };
    std::string relativePaths[] = { "\\",
                                    "\\foo",
                                    "foo/bar/baz"
                                    "C:\\",
                                    "G:",
                                    "R:\\foo",
                                    ""
                                  };
#endif
    for (const auto& str : absolutePaths) {
        FilePath path(str);
        EXPECT_TRUE(path.isAbsolute());
    }
    for (const auto& str : relativePaths) {
        FilePath path(str);
        EXPECT_FALSE(path.isAbsolute());
    }
}

TEST(FilePath, append) {
    const FilePath relative(PATH_LITERAL("foo"));
    FilePath appended;
    // Appending a relative path to an absolute path is fine
    ASSERT_TRUE(sAbsolute.append(relative, &appended));
#ifdef _WIN32
    ASSERT_STREQ(L"C:\\temp\\foo", appended.c_str());
#else
    ASSERT_STREQ("/tmp/foo", appended.c_str());
#endif
    // Cannot append an absolute path to anything
    ASSERT_FALSE(appended.append(sAbsolute, &appended));
    // Cannot store result in nullptr
    ASSERT_FALSE(sAbsolute.append(relative, nullptr));

    // Should be able to append one relative path to another
    ASSERT_TRUE(relative.append(PATH_LITERAL("bar"), &appended));
#ifdef _WIN32
    ASSERT_STREQ(L"foo\\bar", appended.c_str());
#else
    ASSERT_STREQ("foo/bar", appended.c_str());
#endif

    // Should be able to append to one self
    ASSERT_TRUE(relative.append(relative, &appended));
#ifdef _WIN32
    ASSERT_STREQ(L"foo\\foo", appended.c_str());
#else
    ASSERT_STREQ("foo/foo", appended.c_str());
#endif

    // Should be able to append and use one self as destination
    FilePath absoluteCopy(sAbsolute);
    ASSERT_TRUE(absoluteCopy.append(relative, &absoluteCopy));
#ifdef _WIN32
    ASSERT_STREQ(L"C:\\temp\\foo", absoluteCopy.c_str());
#else
    ASSERT_STREQ("/tmp/foo", absoluteCopy.c_str());
#endif
}

TEST(FilePath, concat) {
    const FilePath relative("foo/bar");

    FilePath absolute(sAbsolute);
    FilePath& result = absolute.concat(relative);
    // Should return reference to self
    ASSERT_EQ(&result, &absolute);
    // Should  have made a simple string concatenation, no separators involved
    ASSERT_STREQ((sAbsolute.native() + relative.native()).c_str(),
                 absolute.c_str());

    // Concatenation with an absolute path is allowed, no holds barred
    FilePath concated = relative;
    concated.concat(sAbsolute);
    ASSERT_STREQ((relative.native() + sAbsolute.native()).c_str(),
                 concated.c_str());

    // Concatenating with a native string type is also possible
    concated = relative;
    concated.concat(sAbsolute.native());
    ASSERT_STREQ((relative.native() + sAbsolute.native()).c_str(),
                 concated.c_str());
}

TEST(FilePath, asUtf8) {
#ifdef _WIN32
    // The magic numbers represent the snowman symbol (U+2603)
    FilePath snowmanPath(L"C:\\temp\\\x2603\\snowman");
    std::string snowmanUtf8("C:\\temp\\\xe2\x98\x83\\snowman");
    ASSERT_STREQ(snowmanUtf8.c_str(), snowmanPath.asUtf8().c_str());
#else
    ASSERT_STREQ(sAbsolute.native().c_str(), sAbsolute.asUtf8().c_str());
#endif
}

}  // namespace base
}  // namespace android
