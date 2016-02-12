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

#include "android/base/files/FileSystem.h"

#include "android/base/Uuid.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <random>

namespace android {
namespace base {

// We keep our own set of file related functions in here to make sure that we
// can actually test that our file system operations do something. Depending on
// other parts of the code for this could lead to a reliance on FileSystem
//
// Implementations are at the bottom of the file to keep a focus on the tests

static bool removeDirectory(const FilePath& path);

struct DirectoryRemover {
    template <class T>
    void operator()(T ptr) const {
        EXPECT_TRUE(removeDirectory(ptr));
        free(ptr);
    }
};

// Note that this also owns the char pointer and it's memory will be freed
using ScopedDirectory = ScopedPtr<FilePath::CharType, DirectoryRemover>;

static size_t writeFile(const FilePath& filename,
                        const char* data,
                        size_t size);
static size_t readFile(const FilePath& filename, char* buffer, size_t size);
static std::vector<char> generateData(size_t size);
static ScopedDirectory createTempDirectory();
static bool pathIsFile(const FilePath& path);
static bool fileEqualsData(const FilePath& filename,
                           const char* data,
                           size_t size);


struct {
    const char* dest;
    const char* source;
    std::vector<char> data;
} sTestData[] = {
    { "foo", "bar", generateData(42) },
    { u8"目的地", u8"资源", generateData(123) },
    { "dest", "src", generateData(1024) },
    { u8"назначения", u8"источник", generateData(28) },
    { u8"☃", "snowman", generateData(12) },
};

TEST(FileSystem, get) {
    const FileSystem& fileSystem = FileSystem::get();

    const FileSystem& anotherFileSystem = FileSystem::get();

    EXPECT_EQ(&fileSystem, &anotherFileSystem);
}

TEST(FileSystem, copyFile) {
    ScopedDirectory scopedTempDir = createTempDirectory();
    FilePath tempDir(scopedTempDir.get());

    const FileSystem& fs = FileSystem::get();
    for (const auto& test : sTestData) {
        FilePath dest = tempDir + FilePath(test.dest);
        FilePath source = tempDir +  FilePath(test.source);

        size_t written = writeFile(source, &test.data[0], test.data.size());
        ASSERT_EQ(test.data.size(), written);

        // Copy file and check the data
        ASSERT_TRUE(fs.copyFile(dest, source));
        ASSERT_TRUE(pathIsFile(dest));
        EXPECT_TRUE(fileEqualsData(dest, &test.data[0], test.data.size()));

        // A second copy should be possible and successful
        ASSERT_TRUE(fs.copyFile(dest, source).success());
        EXPECT_TRUE(fileEqualsData(dest, &test.data[0], test.data.size()));
    }
}

TEST(FileSystem, moveFile) {
    ScopedDirectory destDir = createTempDirectory();
    ScopedDirectory sourceDir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    for (const auto& test : sTestData) {
        // Create destination paths in both another directory as well as
        // in the same directory
        FilePath other = FilePath(destDir.get()) + StringView(test.dest);
        FilePath same = FilePath(sourceDir.get()) + StringView(test.dest);
        FilePath source = FilePath(sourceDir.get()) + StringView(test.source);

        // Create a source file
        size_t written = writeFile(source, &test.data[0], test.data.size());
        ASSERT_EQ(written, test.data.size());
        // Move to another directory
        ASSERT_TRUE(fs.moveFile(other, source).success());
        EXPECT_TRUE(fileEqualsData(other, &test.data[0], test.data.size()));
        // Source is gone
        ASSERT_FALSE(pathIsFile(source));

        // Can't move twice, source file should be missing
        EXPECT_FALSE(fs.moveFile(other, source).success());

        // Write a new source file
        written = writeFile(source, &test.data[0], test.data.size());

        // Test move within the same directory
        ASSERT_EQ(written, test.data.size());
        ASSERT_TRUE(fs.moveFile(same, source).success());
        EXPECT_TRUE(fileEqualsData(same, test.data.data(), test.data.size()));
        // Source is gone
        EXPECT_FALSE(pathIsFile(source));
    }
}

TEST(FileSystem, openClose) {
    ScopedDirectory dir = createTempDirectory();
    const FileSystem& fs = FileSystem::get();
    for (const auto& test : sTestData) {
        FilePath path = FilePath(dir.get()) + StringView(test.dest);
        // File should not exist at first so this should fail
        FileHandle handle;
        ASSERT_FALSE(fs.openFile(path, &handle, FileSystem::ModeRead));

        // Open for writing and then closing it should succeed
        ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeWrite));
        ASSERT_TRUE(fs.closeFile(handle));
        EXPECT_TRUE(pathIsFile(path));
    }
}

TEST(FileSystem, basicReads) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    for (const auto& test : sTestData) {
        // Write data to file
        FilePath path = FilePath(dir.get()) + StringView(test.dest);
        size_t written = writeFile(path, &test.data[0], test.data.size());
        ASSERT_EQ(written, test.data.size());

        FileHandle handle;
        ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeRead));
        std::vector<char> buffer(test.data.size() + 1);
        size_t bytesRead = 0;
        EXPECT_TRUE(fs.readFile(handle, &buffer[0], buffer.size(), &bytesRead));
        EXPECT_EQ(test.data.size(), bytesRead);
        ASSERT_TRUE(fs.closeFile(handle));
    }
}

TEST(FileSystem, basicWrites) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    for (const auto& test : sTestData) {
        FilePath path = FilePath(dir.get()) + StringView(test.dest);
        FileHandle handle;
        ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeWrite));
        size_t written = 0;
        EXPECT_TRUE(fs.writeFile(handle, &test.data[0],
                    test.data.size(), &written));
        ASSERT_TRUE(fs.closeFile(handle));

        EXPECT_TRUE(fileEqualsData(path, &test.data[0], test.data.size()));
    }
}

TEST(FileSystem, writeAppend) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    FilePath path = FilePath(dir.get()) + PATH_LITERAL("writeAppend");
    char data[] = "This is some test";
    // Don't write the null terminator to make it easier for us later on
    ASSERT_EQ(sizeof(data) - 1, writeFile(path, data, sizeof(data) - 1));
    FileHandle handle;
    ASSERT_TRUE(fs.openFile(path, &handle,
                            FileSystem::ModeWrite | FileSystem::ModeAppend));
    size_t written = 0;
    ASSERT_TRUE(fs.writeFile(handle, " data", 5, &written));
    EXPECT_EQ(5, written);
    ASSERT_TRUE(fs.closeFile(handle));

    char expectedData[] = "This is some test data";
    // We didn't write the null terminator so drop one byte at the end
    EXPECT_TRUE(fileEqualsData(path, expectedData, sizeof(expectedData) - 1));
}

TEST(FileSysten, writeTruncate) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    FilePath path = FilePath(dir.get()) + PATH_LITERAL("writeTruncate");
    char data[] = "This poor data will all be gone soon";
    ASSERT_EQ(sizeof(data), writeFile(path, data, sizeof(data)));
    FileHandle handle;
    ASSERT_TRUE(fs.openFile(path, &handle,
                            FileSystem::ModeWrite | FileSystem::ModeTruncate));
    char newData[] = "This will replace everything!";
    size_t written = 0;
    ASSERT_TRUE(fs.writeFile(handle, newData, sizeof(newData), &written));
    EXPECT_EQ(sizeof(newData), written);
    ASSERT_TRUE(fs.closeFile(handle));

    EXPECT_TRUE(fileEqualsData(path, newData, sizeof(newData)));
}

TEST(FileSystem, seekReads) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    FilePath path = FilePath(dir.get()) + PATH_LITERAL("seekReads");
    const char data[] = "Seek and you shall find";
    ASSERT_GT(writeFile(path, data, sizeof(data)), 0);

    FileHandle handle;
    ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeRead));
    // Skip one byte from the start
    ASSERT_TRUE(fs.seekFile(handle, 1, FileSystem::SeekSet));
    size_t bytesRead;
    char buffer[128] = {};
    ASSERT_TRUE(fs.readFile(handle, buffer, sizeof(buffer), &bytesRead));
    EXPECT_EQ(sizeof(data) - 1, bytesRead);
    EXPECT_STREQ("eek and you shall find", buffer);

    // Test seeking from current, but first we have to reset the position
    // to somewhere.
    ASSERT_TRUE(fs.seekFile(handle, 1, FileSystem::SeekSet));
    // Seek four characters further
    ASSERT_TRUE(fs.seekFile(handle, 4, FileSystem::SeekCur));
    ASSERT_TRUE(fs.readFile(handle, buffer, sizeof(buffer), &bytesRead));
    EXPECT_EQ(sizeof(data) - 5, bytesRead);
    EXPECT_STREQ("and you shall find", buffer);

    // Test seeking from the end
    ASSERT_TRUE(fs.seekFile(handle, -5, FileSystem::SeekEnd));
    ASSERT_TRUE(fs.readFile(handle, buffer, sizeof(buffer), &bytesRead));
    EXPECT_EQ(5, bytesRead);
    EXPECT_STREQ("find", buffer);

    // Test seeking from current with a negative offset
    ASSERT_TRUE(fs.seekFile(handle, -2, FileSystem::SeekCur));
    ASSERT_TRUE(fs.readFile(handle, buffer, sizeof(buffer), &bytesRead));
    EXPECT_EQ(2, bytesRead);
    EXPECT_STREQ("d", buffer);
    ASSERT_TRUE(fs.closeFile(handle));
}

TEST(FileSystem, seekWrites) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();
    FilePath path = FilePath(dir.get()) + PATH_LITERAL("seekWrites");
    const char data[] = "Write and it shall be so";
    ASSERT_GT(writeFile(path, data, sizeof(data)), 0);
    FileHandle handle;
    ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeWrite));
    ASSERT_TRUE(fs.seekFile(handle, 6, FileSystem::SeekSet));
    size_t written = 0;
    ASSERT_TRUE(fs.writeFile(handle, "foo", 3, &written));
    ASSERT_TRUE(fs.closeFile(handle));
    const char adjustedData[] = "Write foo it shall be so";
    EXPECT_TRUE(fileEqualsData(path, adjustedData, sizeof(adjustedData)));

    ASSERT_TRUE(fs.openFile(path, &handle, FileSystem::ModeWrite));
    ASSERT_TRUE(fs.seekFile(handle, -2, FileSystem::SeekEnd));
    ASSERT_TRUE(fs.writeFile(handle, "q", 1, &written));
    EXPECT_EQ(1, written);
    ASSERT_TRUE(fs.seekFile(handle, -11, FileSystem::SeekCur));
    ASSERT_TRUE(fs.writeFile(handle, "could", 5, &written));
    EXPECT_EQ(5, written);
    ASSERT_TRUE(fs.closeFile(handle));
    const char mangledData[] = "Write foo it could be sq";
    EXPECT_TRUE(fileEqualsData(path, mangledData, sizeof(mangledData)));
}

TEST(FileSystem, seekReadAndWrites) {
    ScopedDirectory dir = createTempDirectory();

    const FileSystem& fs = FileSystem::get();

    FilePath path = FilePath(dir.get()) + PATH_LITERAL("seekReadAndWrites");
    const char data[] = "The lazy dog was overjumped by some kind of fox";

    FileHandle handle;
    ASSERT_TRUE(fs.openFile(path, &handle,
                            FileSystem::ModeRead | FileSystem::ModeWrite));
    size_t written = 0;
    ASSERT_TRUE(fs.writeFile(handle, data, sizeof(data), &written));
    EXPECT_EQ(sizeof(data), written);

    // Move back past the null-terminator to the beginning of "fox"
    ASSERT_TRUE(fs.seekFile(handle, -4, FileSystem::SeekCur));
    char buffer[128] = {};
    size_t bytesRead = 0;
    ASSERT_TRUE(fs.readFile(handle, buffer, sizeof(buffer), &bytesRead));
    EXPECT_EQ(4, bytesRead);
    EXPECT_EQ(0, memcmp("fox", buffer, 4));

    ASSERT_TRUE(fs.seekFile(handle, 4, FileSystem::SeekSet));
    ASSERT_TRUE(fs.writeFile(handle, "blue", 4, &written));
    EXPECT_EQ(4, written);
    ASSERT_TRUE(fs.closeFile(handle));

    char expectedData[] = "The blue dog was overjumped by some kind of fox";
    EXPECT_TRUE(fileEqualsData(path, expectedData, sizeof(expectedData)));
}

static size_t writeFile(const FilePath& filename, const char* data, size_t size) {
#ifdef _WIN32
    FILE* file = _wfopen(filename.c_str(), L"wb");
#else
    FILE* file = fopen(filename.c_str(), "w");
#endif
    if (file == nullptr) {
        return 0;
    }

    size_t written = fwrite(data, 1, size, file);
    if (fclose(file) != 0) {
        return 0;
    }
    return written;
}


static size_t readFile(const FilePath& filename, char* buffer, size_t size) {
#ifdef _WIN32
    FILE* file = _wfopen(filename.c_str(), L"rb");
#else
    FILE* file = fopen(filename.c_str(), "r");
#endif
    if (file == nullptr) {
        return 0;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    if (fclose(file) != 0) {
        return 0;
    }
    return bytesRead;
}

bool createDirectory(const FilePath& name) {
#ifdef _WIN32
    return _wmkdir(name.c_str()) == 0;
#else
    return mkdir(name.c_str(), 0700) == 0;
#endif
}

static bool pathIsFile(const FilePath& path) {
#ifdef _WIN32
    struct _stat fileStat;
    int result = _wstat(path.c_str(), &fileStat);
#else
    struct stat fileStat;
    int result = stat(path.c_str(), &fileStat);
#endif
    if (result != 0) {
        return false;
    }
    return S_ISREG(fileStat.st_mode);
}

static bool remove(const FilePath& path) {
#ifdef _WIN32
    if (pathIsFile(path)) {
        return ::_wremove(path.c_str()) == 0;
    } else {
        return ::_wrmdir(path.c_str()) == 0;
    }
#else
    return ::remove(path.c_str()) == 0;
#endif
}

static bool removeDirectory(const FilePath& path) {
    StringVector files = System::get()->scanDirEntries(path.asUtf8(), true);
    for (const auto& file : files) {
        if (!remove(FilePath(file))) {
            return false;
        }
    }
    if (!remove(path)) {
        return false;
    }
    return true;
}

static ScopedDirectory createTempDirectory() {
    FilePath tempDir = FilePath(System::get()->getTempDir()) +
                       FilePath(Uuid::generateFast().toString());
    if (!createDirectory(tempDir)) {
        return ScopedDirectory();
    }
#ifdef _WIN32
    return ScopedDirectory(wcsdup(tempDir.c_str()));
#else
    return ScopedDirectory(strdup(tempDir.c_str()));
#endif
}

static bool fileEqualsData(const FilePath& filename, const char* data, size_t size) {
    // Add some extra size in case the file is longer
    std::vector<char> buffer(size + 1, 0);
    size_t bytesRead = readFile(filename, &buffer[0], buffer.size());
    return size == bytesRead && ::memcmp(data, &buffer[0], size) == 0;
}

// Use the same seed all the time, this may seem odd but we really want
// unit tests to be repeatable and this way the data should stay the same
// as long as the code stays the same
std::mt19937 sRng((1u << 31) - 1);

static std::vector<char> generateData(size_t size) {
    std::vector<char> randomData(size);
    std::uniform_int_distribution<> dist(0, 255);
    for (size_t  i ; i < size; ++i) {
        randomData[i] = dist(sRng);
    }
    return randomData;
}

}  // namespace base
}  // namespace android
