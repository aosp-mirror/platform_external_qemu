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

#include <gtest/gtest.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DUP(x) x, x

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

    for (auto n : kData) {
        int ch = n.ch;
        EXPECT_EQ(n.expected[kHostPosix],
                  PathUtils::isDirSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(n.expected[kHostWin32],
                  PathUtils::isDirSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(n.expected[kHostType], PathUtils::isDirSeparator(ch))
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

    for (auto n : kData) {
        int ch = n.ch;
        EXPECT_EQ(n.expected[kHostPosix],
                  PathUtils::isPathSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(n.expected[kHostWin32],
                  PathUtils::isPathSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(n.expected[kHostType], PathUtils::isPathSeparator(ch))
                << "Testing '" << ch << "'";
    }
}

TEST(PathUtils, getDirSeparator) {
    EXPECT_EQ('/', PathUtils::getDirSeparator(kHostPosix));
    EXPECT_EQ('\\', PathUtils::getDirSeparator(kHostWin32));
}

TEST(PathUtils, removeTrailingDirSeparator) {
    static const struct {
        const char* path;
        const char* expected[kHostTypeCount];
    } kData[] = {{"/", {"/", "/"}},
                 {"\\", {"\\", "\\"}},
                 {"foo", {"foo", "foo"}},
                 {"foo/bar", {"foo/bar", "foo/bar"}},
                 {"foo\\bar", {"foo\\bar", "foo\\bar"}},
                 {"foo/bar/", {"foo/bar", "foo/bar"}},
                 {"foo\\bar\\", {"foo\\bar\\", "foo\\bar"}},
                 {"/foo", {"/foo", "/foo"}},
                 {"/foo/", {"/foo", "/foo"}},
                 {"/foo/bar", {"/foo/bar", "/foo/bar"}},
                 {"/foo/bar/", {"/foo/bar", "/foo/bar"}},
                 {"\\foo", {"\\foo", "\\foo"}},
                 {"\\foo\\bar", {"\\foo\\bar", "\\foo\\bar"}},
                 {R"(\foo\bar\)", {R"(\foo\bar\)", "\\foo\\bar"}}};

    for (const auto& n : kData) {
        std::string path;
        std::string input = n.path;
        path = PathUtils::removeTrailingDirSeparator(input, kHostPosix);
        EXPECT_STREQ(n.expected[kHostPosix], path.c_str())
                << "Testing '" << input.c_str() << "'";

        path = PathUtils::removeTrailingDirSeparator(input, kHostWin32);
        EXPECT_STREQ(n.expected[kHostWin32], path.c_str())
                << "Testing '" << input.c_str() << "'";

        path = PathUtils::removeTrailingDirSeparator(input);
        EXPECT_STREQ(n.expected[kHostType], path.c_str())
                << "Testing '" << input.c_str() << "'";
    }
}

TEST(PathUtils, rootPrefixSize) {
    static const struct {
        const char* path;
        size_t prefixSize[kHostTypeCount];
    } kData[] = {
            {nullptr, {0u, 0u}},      {"", {0u, 0u}},
            {"foo", {0u, 0u}},        {"foo/bar", {0u, 0u}},
            {"/foo", {1u, 1u}},       {"//foo", {1u, 5u}},
            {"//foo/bar", {1u, 6u}},  {"c:", {0u, 2u}},
            {"c:foo", {0u, 2u}},      {"c/foo", {0u, 0u}},
            {"c:/foo", {0u, 3u}},     {"c:\\", {0u, 3u}},
            {"c:\\\\", {0u, 3u}},     {"1:/foo", {0u, 0u}},
            {"\\", {0u, 1u}},         {"\\foo", {0u, 1u}},
            {"\\foo\\bar", {0u, 1u}}, {"\\\\foo", {0u, 5u}},
            {R"(\\foo\)", {0u, 6u}},  {R"(\\foo\\bar)", {0u, 6u}},
    };
    for (const auto& n : kData) {
        const char* path = n.path;
        EXPECT_EQ(n.prefixSize[kHostPosix],
                  PathUtils::rootPrefixSize(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(n.prefixSize[kHostWin32],
                  PathUtils::rootPrefixSize(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(n.prefixSize[kHostType], PathUtils::rootPrefixSize(path))
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
    for (auto n : kData) {
        const char* path = n.path;
        EXPECT_EQ(n.expected[kHostPosix],
                  PathUtils::isAbsolute(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(n.expected[kHostWin32],
                  PathUtils::isAbsolute(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(n.expected[kHostType], PathUtils::isAbsolute(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

TEST(PathUtils, split) {
    static const struct {
        const char* path;
        struct {
            bool result;
            const char* dirname;
            const char* basename;
        } expected[kHostTypeCount];
    } kData[] = {
            {".", {{true, ".", "."}, {true, ".", "."}}},
            {"/", {{false, nullptr, nullptr}, {false, nullptr, nullptr}}},
            {"foo", {{true, ".", "foo"}, {true, ".", "foo"}}},
            {"/foo", {{true, "/", "foo"}, {true, "/", "foo"}}},
            {"foo/bar", {{true, "foo/", "bar"}, {true, "foo/", "bar"}}},
            {"foo\\bar", {{true, ".", "foo\\bar"}, {true, "foo\\", "bar"}}},
            {"foo/", {{false, nullptr, nullptr}, {false, nullptr, nullptr}}},
            {"foo\\", {{true, ".", "foo\\"}, {false, nullptr, nullptr}}},
            {"C:foo", {{true, ".", "C:foo"}, {true, "C:", "foo"}}},
            {"C:/foo", {{true, "C:/", "foo"}, {true, "C:/", "foo"}}},
    };
    for (const auto& n : kData) {
        StringView dirname, basename;

        EXPECT_EQ(n.expected[kHostPosix].result,
                  PathUtils::split(n.path, kHostPosix, &dirname, &basename))
                << "when testing posix [" << n.path << "]";
        if (n.expected[kHostPosix].result) {
            EXPECT_EQ(n.expected[kHostPosix].dirname, dirname)
                    << "when testing posix [" << n.path << "]";
            EXPECT_EQ(n.expected[kHostPosix].basename, basename)
                    << "when testing posix [" << n.path << "]";
        }

        EXPECT_EQ(n.expected[kHostWin32].result,
                  PathUtils::split(n.path, kHostWin32, &dirname, &basename))
                << "when testing win32 [" << n.path << "]";
        if (n.expected[kHostWin32].result) {
            EXPECT_EQ(n.expected[kHostWin32].dirname, dirname)
                    << "when testing win32 [" << n.path << "]";
            EXPECT_EQ(n.expected[kHostWin32].basename, basename)
                    << "when testing win32 [" << n.path << "]";
        }

        EXPECT_EQ(n.expected[kHostType].result,
                  PathUtils::split(n.path, &dirname, &basename))
                << "when testing host [" << n.path << "]";
        if (n.expected[kHostType].result) {
            EXPECT_EQ(n.expected[kHostType].dirname, dirname)
                    << "when testing host [" << n.path << "]";
            EXPECT_EQ(n.expected[kHostType].basename, basename)
                    << "when testing host [" << n.path << "]";
        }
    }
}

TEST(PathUtils, join) {
    static const struct {
        const char* path1;
        const char* path2;
        const char* expected[kHostTypeCount];
    } kData[] = {{".", "foo", {"./foo", ".\\foo"}},
                 {"foo", "/bar", {"/bar", "/bar"}},
                 {"foo", "bar", {"foo/bar", "foo\\bar"}},
                 {"foo/", "bar", {"foo/bar", "foo/bar"}},
                 {"foo\\", "bar", {"foo\\/bar", "foo\\bar"}},
                 {"C:", "foo", {"C:/foo", "C:foo"}},
                 {"C:/", "foo", {"C:/foo", "C:/foo"}},
                 {"C:\\", "foo", {"C:\\/foo", "C:\\foo"}}};
    for (const auto& n : kData) {
        std::string path = PathUtils::join(n.path1, n.path2, kHostPosix);
        EXPECT_STREQ(n.expected[kHostPosix], path.c_str())
                << "When testing posix [" << n.path1 << ", " << n.path2 << "]";

        path = PathUtils::join(n.path1, n.path2, kHostWin32);
        EXPECT_STREQ(n.expected[kHostWin32], path.c_str())
                << "When testing win32 [" << n.path1 << ", " << n.path2 << "]";

        path = PathUtils::join(n.path1, n.path2);
        EXPECT_STREQ(n.expected[kHostType], path.c_str())
                << "When testing host [" << n.path1 << ", " << n.path2 << "]";
    }
}

static const int kMaxComponents = 10;

typedef const char* ComponentList[kMaxComponents];

static void checkComponents(const ComponentList& expected,
                            const std::vector<StringView>& components,
                            const char* hostType,
                            const char* path) {
    size_t m;
    for (m = 0; m < components.size(); ++m) {
        if (!expected[m]) {
            break;
        }
        const char* component = expected[m];
        EXPECT_EQ(component, components[m])
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
            {"", {{nullptr}, {nullptr}}},
            {"foo", {{"foo", nullptr}, {"foo", nullptr}}},
            {"foo/", {{"foo", nullptr}, {"foo", nullptr}}},
            {"foo/bar", {{"foo", "bar", nullptr}, {"foo", "bar", nullptr}}},
            {"foo//bar/zoo",
             {{"foo", "bar", "zoo", nullptr}, {"foo", "bar", "zoo", nullptr}}},
            {R"(\foo\bar\)",
             {{R"(\foo\bar\)", nullptr}, {"\\", "foo", "bar", nullptr}}},
            {"C:foo\\bar",
             {{"C:foo\\bar", nullptr}, {"C:", "foo", "bar", nullptr}}},
            {"C:/foo", {{"C:", "foo", nullptr}, {"C:/", "foo", nullptr}}},
            {"/foo", {{"/", "foo", nullptr}, {"/", "foo", nullptr}}},
            {"\\foo", {{"\\foo", nullptr}, {"\\", "foo", nullptr}}},
    };
    for (const auto& n : kData) {
        const char* path = n.path;
        checkComponents(n.components[kHostPosix],
                        PathUtils::decompose(path, kHostPosix), "posix", path);

        checkComponents(n.components[kHostWin32],
                        PathUtils::decompose(path, kHostWin32), "win32", path);

        checkComponents(n.components[kHostType], PathUtils::decompose(path),
                        "host", path);
    }
}

static std::vector<std::string> componentListToVector(
        const ComponentList& input) {
    std::vector<std::string> result;
    for (size_t i = 0; input[i]; ++i) {
        result.emplace_back(input[i]);
    }
    return result;
}

TEST(PathUtils, recompose) {
    static const struct {
        const ComponentList input;
        const char* path[kHostTypeCount];
    } kData[] = {
            {{nullptr}, {"", ""}},
            {{".", nullptr}, {".", "."}},
            {{"..", nullptr}, {"..", ".."}},
            {{"/", nullptr}, {"/", "/"}},
            {{"/", "foo", nullptr}, {"/foo", "/foo"}},
            {{"\\", "foo", nullptr}, {"\\/foo", "\\foo"}},
            {{"foo", nullptr}, {"foo", "foo"}},
            {{"foo", "bar", nullptr}, {"foo/bar", "foo\\bar"}},
            {{".", "foo", "..", nullptr}, {"./foo/..", ".\\foo\\.."}},
            {{"C:", "foo", nullptr}, {"C:/foo", "C:foo"}},
            {{"/foo/bar", "lib", nullptr}, {"/foo/bar/lib", "/foo/bar\\lib"}},
            {{"\\foo\\bar", "lib", nullptr},
             {"\\foo\\bar/lib", R"(\foo\bar\lib)"}},
    };
    for (const auto& n : kData) {
        std::vector<std::string> components = componentListToVector(n.input);
        EXPECT_STREQ(n.path[kHostPosix],
                     PathUtils::recompose(components, kHostPosix).c_str());
        EXPECT_STREQ(n.path[kHostWin32],
                     PathUtils::recompose(components, kHostWin32).c_str());
        EXPECT_STREQ(n.path[kHostType],
                     PathUtils::recompose(components).c_str());
    }
}


// Convert a vector of strings |components| into a file path, using
// |separator| as the directory separator.
static std::string componentsToPath(const ComponentList& components,
                                    char separator) {
    std::string result;
    for (size_t n = 0; components[n]; ++n) {
        if (n) {
            result += separator;
        }
        result += components[n];
    }
    return result;
}

static std::string stringVectorToPath(const std::vector<std::string>& input,
                                      char separator) {
    std::string result;
    for (size_t n = 0; n < input.size(); ++n) {
        if (n) {
            result += separator;
        }
        result += input[n];
    }
    return result;
}

TEST(PathUtils, simplifyComponents) {
    static const struct {
        const ComponentList input;
        const ComponentList expected;
    } kData[] = {
            {{nullptr}, {".", nullptr}},
            {{".", nullptr}, {".", nullptr}},
            {{"..", nullptr}, {"..", nullptr}},
            {{"..", "..", nullptr}, {"..", "..", nullptr}},
            {{"foo", nullptr}, {"foo", nullptr}},
            {{"foo", ".", nullptr}, {"foo", nullptr}},
            {{"foo", "bar", nullptr}, {"foo", "bar", nullptr}},
            {{".", "foo", ".", "bar", ".", nullptr}, {"foo", "bar", nullptr}},
            {{"foo", "..", "bar", nullptr}, {"bar", nullptr}},
            {{".", "..", "foo", "bar", nullptr}, {"..", "foo", "bar", nullptr}},
            {{"..", "foo", "..", "bar", nullptr}, {"..", "bar", nullptr}},
            {{"foo", "..", "..", "bar", nullptr}, {"..", "bar", nullptr}},
    };
    for (const auto& n : kData) {
        const ComponentList& input = n.input;
        std::string inputPath = componentsToPath(input, '!');
        std::string expectedPath = componentsToPath(n.expected, '!');
        std::vector<std::string> components = componentListToVector(input);
        PathUtils::simplifyComponents(&components);
        std::string path = stringVectorToPath(components, '!');

        EXPECT_STREQ(expectedPath.c_str(), path.c_str())
            << "When simplifying " << inputPath.c_str();
    }
}

TEST(PathUtils, toExecutableName) {
    static const struct {
        PathUtils::HostType host;
        const char* input;
        const char* expected;
    } kData[] = {
        { PathUtils::HOST_POSIX, "blah", "blah" },
        { PathUtils::HOST_WIN32, "blah", "blah.exe" },
    };
    for (const auto& n : kData) {
        EXPECT_STREQ(n.expected,
                     PathUtils::toExecutableName(n.input, n.host).c_str());

        // make sure the overload without host parameter works correctly too
        if (n.host == PathUtils::HOST_TYPE) {
            EXPECT_STREQ(PathUtils::toExecutableName(n.input, n.host).c_str(),
                         PathUtils::toExecutableName(n.input).c_str());
        }
    }
}

TEST(PathUtils, extension) {
    static const struct {
        PathUtils::HostType host;
        const char* input;
        const char* expected;
    } kData[] = {
        { PathUtils::HOST_POSIX, "", "" },
        { PathUtils::HOST_POSIX, "name.ext", ".ext" },
        { PathUtils::HOST_POSIX, "name.", "." },
        { PathUtils::HOST_POSIX, ".", "." },
        { PathUtils::HOST_POSIX, "dir/path", "" },
        { PathUtils::HOST_POSIX, "dir/path.ext", ".ext" },
        { PathUtils::HOST_POSIX, "dir.ext/path", "" },
        { PathUtils::HOST_POSIX, ".name", ".name" },

        { PathUtils::HOST_WIN32, "", "" },
        { PathUtils::HOST_WIN32, "name.ext", ".ext" },
        { PathUtils::HOST_WIN32, "name.", "." },
        { PathUtils::HOST_WIN32, ".", "." },
        { PathUtils::HOST_WIN32, "dir\\path", "" },
        { PathUtils::HOST_WIN32, "dir\\path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir/path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir\\path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir.bad//path", "" },
        { PathUtils::HOST_WIN32, "dir.bad\\path", "" },
        { PathUtils::HOST_WIN32, ".name", ".name" },
        { PathUtils::HOST_WIN32, "c:\\path..ext", ".ext" },
    };
    for (const auto& n : kData) {
        EXPECT_STREQ(n.expected, PathUtils::extension(n.input, n.host).c_str());

        // make sure the overload without host parameter works correctly too
        if (n.host == PathUtils::HOST_TYPE) {
            EXPECT_STREQ(PathUtils::extension(n.input, n.host).c_str(),
                         PathUtils::extension(n.input).c_str());
        }
    }
}

}  // namespace base
}  // namespace android
