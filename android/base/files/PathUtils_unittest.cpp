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

#include "android/base/files/PathUtils.h"

#include "android/base/containers/StringVector.h"
#include "android/base/String.h"

#include <gtest/gtest.h>

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

static const int kHostTypeCount = PathUtils::kHostTypeCount;

TEST(PathUtils, isDirSeparator) {
    static const struct {
        int ch;
        bool expected[kHostTypeCount];
    } kData[] = {
        { '/', { true, true }},
        { '\\', { false, true }},
        { '$', { false, false }},
        { ':', { false, false }},
        { ';', { false, false }},
    };

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        int ch = kData[n].ch;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isDirSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isDirSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isDirSeparator(ch))
                << "Testing '" << ch << "'";
    }
}

TEST(PathUtils, isPathSeparator) {
    static const struct {
        int ch;
        bool expected[kHostTypeCount];
    } kData[] = {
        { ':', { true, false }},
        { ';', { false, true }},
        { '/', { false, false }},
        { '\\', { false, false }},
        { '$', { false, false }},
    };

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        int ch = kData[n].ch;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isPathSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isPathSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isPathSeparator(ch))
                << "Testing '" << ch << "'";
    }
}

TEST(PathUtils, rootPrefixSize) {
    static const struct {
        const char* path;
        size_t prefixSize[kHostTypeCount];
    } kData[] = {
        { NULL, { 0u, 0u} },
        { "", { 0u, 0u } },
        { "foo", { 0u, 0u } },
        { "foo/bar", { 0u, 0u } },
        { "/foo", { 1u, 1u } },
        { "//foo", { 1u, 5u } },
        { "//foo/bar", { 1u, 6u } },
        { "c:", { 0u, 2u } },
        { "c:foo", { 0u, 2u } },
        { "c/foo", { 0u, 0u } },
        { "c:/foo", { 0u, 3u } },
        { "c:\\", { 0u, 3u } },
        { "c:\\\\", { 0u, 3u } },
        { "1:/foo", { 0u, 0u } },
        { "\\", { 0u, 1u } },
        { "\\foo", { 0u, 1u } },
        { "\\foo\\bar", { 0u, 1u } },
        { "\\\\foo", { 0u, 5u } },
        { "\\\\foo\\", { 0u, 6u } },
        { "\\\\foo\\\\bar", { 0u, 6u } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        EXPECT_EQ(kData[n].prefixSize[kHostPosix],
                  PathUtils::rootPrefixSize(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].prefixSize[kHostWin32],
                  PathUtils::rootPrefixSize(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].prefixSize[kHostType],
                  PathUtils::rootPrefixSize(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

TEST(PathUtils, isAbsolute) {
    static const struct {
        const char* path;
        bool expected[kHostTypeCount];
    } kData[] = {
        { "foo", { false, false } },
        { "/foo", { true, true } },
        { "\\foo", { false, true } },
        { "/foo/bar", { true, true } },
        { "\\foo\\bar", { false, true } },
        { "C:foo", { false, false } },
        { "C:/foo", { false, true } },
        { "C:\\foo", { false, true } },
        { "//server", { true, false } },
        { "//server/path", { true, true } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isAbsolute(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isAbsolute(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isAbsolute(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

static const int kMaxComponents = 10;

typedef const char* ComponentList[kMaxComponents];

static void checkComponents(const ComponentList& expected,
                            const StringVector& components,
                            const char* hostType,
                            const char* path) {
    size_t m;
    for (m = 0; m < components.size(); ++m) {
        if (!expected[m])
            break;
        const char* component = expected[m];
        EXPECT_STREQ(component, components[m].c_str())
                << hostType << " component #" << (m + 1) << " in " << path;
    }
    EXPECT_EQ(m, components.size())
                << hostType << " component #" << (m + 1) << " in " << path;
}

TEST(PathUtils, decompose) {
    static const struct {
        const char* path;
        const ComponentList components[kHostTypeCount];
    } kData[] = {
        { "", { { NULL }, { NULL } } },
        { "foo", {
            { "foo", NULL },
            { "foo", NULL } } },
        { "foo/", {
            { "foo", NULL },
            { "foo", NULL } } },
        { "foo/bar", {
            { "foo", "bar", NULL },
            { "foo", "bar", NULL } } },
        { "foo//bar/zoo", {
            { "foo", "bar", "zoo", NULL },
            { "foo", "bar", "zoo", NULL } } },
        { "\\foo\\bar\\", {
            { "\\foo\\bar\\", NULL },
            { "\\", "foo", "bar", NULL } } },
        { "C:foo\\bar", {
            { "C:foo\\bar", NULL },
            { "C:", "foo", "bar", NULL } } },
        { "C:/foo", {
            { "C:", "foo", NULL },
            { "C:/", "foo", NULL } } },
        { "/foo", {
            { "/", "foo", NULL },
            { "/", "foo", NULL } } },
        { "\\foo", {
            { "\\foo", NULL },
            { "\\", "foo", NULL } } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        checkComponents(kData[n].components[kHostPosix],
                        PathUtils::decompose(path, kHostPosix),
                        "posix",
                        path);

        checkComponents(kData[n].components[kHostWin32],
                        PathUtils::decompose(path, kHostWin32),
                        "win32",
                        path);

        checkComponents(kData[n].components[kHostType],
                        PathUtils::decompose(path),
                        "host",
                        path);
    }
}

static StringVector componentListToVector(
        const ComponentList& input) {
    StringVector result;
    for (size_t i = 0; input[i]; ++i)
        result.push_back(input[i]);
    return result;
}

TEST(PathUtils, recompose) {
    static const struct {
        const ComponentList input;
        const char* path[kHostTypeCount];
    } kData[] = {
        { { NULL }, { "", "" } },
        { { ".", NULL }, { ".", "." } },
        { { "..", NULL }, { "..", ".." } },
        { { "/", NULL }, { "/", "/" } },
        { { "/", "foo", NULL }, { "/foo", "/foo" } },
        { { "\\", "foo", NULL }, { "\\/foo", "\\foo" } },
        { { "foo", NULL }, { "foo", "foo" } },
        { { "foo", "bar", NULL }, { "foo/bar", "foo\\bar" } },
        { { ".", "foo", "..", NULL }, { "./foo/..", ".\\foo\\.." } },
        { { "C:", "foo", NULL }, { "C:/foo", "C:foo" } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        StringVector components = componentListToVector(kData[n].input);
        EXPECT_STREQ(kData[n].path[kHostPosix],
                     PathUtils::recompose(components, kHostPosix).c_str());
        EXPECT_STREQ(kData[n].path[kHostWin32],
                     PathUtils::recompose(components, kHostWin32).c_str());
        EXPECT_STREQ(kData[n].path[kHostType],
                     PathUtils::recompose(components).c_str());
    }
}


// Convert a vector of strings |components| into a file path, using
// |separator| as the directory separator.
static String componentsToPath(
        const ComponentList& components,
        char separator) {
    String result;
    for (size_t n = 0; components[n]; ++n) {
        if (n)
            result += separator;
        result += components[n];
    }
    return result;
}

static String stringVectorToPath(
        const StringVector& input,
        char separator) {
    String result;
    for (size_t n = 0; n < input.size(); ++n) {
        if (n)
            result += separator;
        result += input[n];
    }
    return result;
}

TEST(PathUtils, simplifyComponents) {
    static const struct {
        const ComponentList input;
        const ComponentList expected;
    } kData[] = {
        { { NULL }, { ".", NULL } },
        { { ".", NULL }, { ".", NULL } },
        { { "..", NULL }, { "..", NULL } },
        { { "foo", NULL }, { "foo", NULL } },
        { { "foo", ".", NULL }, { "foo", NULL } },
        { { "foo", "bar", NULL }, { "foo", "bar", NULL } },
        { { ".", "foo", ".", "bar", ".", NULL }, { "foo", "bar", NULL } },
        { { "foo", "..", "bar", NULL }, { "bar", NULL } },
        { { ".", "..", "foo", "bar", NULL }, { "..", "foo", "bar", NULL } },
        { { "..", "foo", "..", "bar", NULL }, { "..", "bar", NULL } },
        { { "foo", "..", "..", "bar", NULL }, { "..", "bar", NULL } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const ComponentList& input = kData[n].input;
        String inputPath = componentsToPath(input, '!');
        String expectedPath = componentsToPath(kData[n].expected, '!');
        StringVector components = componentListToVector(input);
        PathUtils::simplifyComponents(&components);
        String path = stringVectorToPath(components, '!');

        EXPECT_STREQ(expectedPath.c_str(), path.c_str())
            << "When simplifying " << inputPath.c_str();
    }
};

}  // namespace android
}  // namespace base
