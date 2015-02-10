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

#ifndef ANDROID_BASE_TESTING_TEST_TEMP_DIR_H
#define ANDROID_BASE_TESTING_TEST_TEMP_DIR_H

#include "android/base/Compiler.h"
#include "android/base/Log.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#include <errno.h>
#include <stdio.h>
#endif

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace android {
namespace base {

// A class used to model a temporary directory used during testing.
// Usage is simple:
//
//      {
//        TestTempDir myDir("my_test");   // creates new temp directory.
//        ASSERT_TRUE(myDir.path());      // NULL if error during creation.
//        ... write files into directory path myDir->path()
//        ... do your test
//      }   // destructor removes temp directory and all files under it.

class TestTempDir {
public:
    // Create new instance. This also tries to create a new temporary
    // directory. |debugPrefix| is an optional name prefix and can be NULL.
    TestTempDir(const char* debugName) {
#ifdef _WIN32
        String temp_dir = getTempPath();
#else  // !_WIN32
        String temp_dir = "/tmp/";
#endif  // !_WIN32
        if (debugName) {
            temp_dir += debugName;
            temp_dir += ".";
        }
        temp_dir += "XXXXXX";
        if (mkdtemp(&temp_dir[0])) {
            mPath = temp_dir;
        }
    }

    // Return the path to the temporary directory, or NULL if it could not
    // be created for some reason.
    const char* path() const { return mPath.size() ? mPath.c_str() : NULL; }

    // Return the path as a String. It will be empty if the directory could
    // not be created for some reason.
    const String& pathString() const { return mPath; }

    // Destroy instance, and removes the temporary directory and all files
    // inside it.
    ~TestTempDir() {
        if (mPath.size()) {
            DeleteRecursive(mPath);
        }
    }

    // Create the path of a directory entry under the temporary directory.
    String makeSubPath(const char* subpath) {
        return StringFormat("%s/%s", mPath.c_str(), subpath);
    }

    // Create an empty directory under the temporary directory.
    bool makeSubDir(const char* subdir) {
        String path = makeSubPath(subdir);
#ifdef _WIN32
        if (mkdir(path.c_str()) < 0) {
            PLOG(ERROR) << "Can't create " << path.c_str() << ": ";
            return false;
        }
#else
        if (mkdir(path.c_str(), 0755) < 0) {
            PLOG(ERROR) << "Can't create " << path.c_str() << ": ";
            return false;
        }
#endif
        return true;
    }

    // Create an empty file under the temporary directory.
    bool makeSubFile(const char* file) {
        String path = makeSubPath(file);
        int fd = ::open(path.c_str(), O_WRONLY|O_CREAT);
        if (fd < 0) {
            PLOG(ERROR) << "Can't create " << path.c_str() << ": ";
            return false;
        }
        ::close(fd);
        return true;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(TestTempDir);

    void DeleteRecursive(const String& path) {
        // First remove any files in the dir
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return;
        }

        dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
                continue;
            }
            String entry_path = StringFormat("%s/%s", path.c_str(), entry->d_name);
            struct stat stats;
            lstat(entry_path.c_str(), &stats);
            if (S_ISDIR(stats.st_mode)) {
                DeleteRecursive(entry_path);
            } else {
                unlink(entry_path.c_str());
            }
        }
        closedir(dir);
        rmdir(path.c_str());
    }

#ifdef _WIN32
    String getTempPath() {
        String result;
        DWORD len = GetTempPath(0, NULL);
        if (!len) {
            LOG(FATAL) << "Can't find temporary path!";
        }
        result.resize(static_cast<size_t>(len));
        GetTempPath(len, &result[0]);
        // The length returned by GetTempPath() is sometimes too large.
        result.resize(::strlen(result.c_str()));
        for (size_t n = 0; n < result.size(); ++n) {
            if (result[n] == '\\') {
                result[n] = '/';
            }
        }
        if (result.size() && result[result.size() - 1] != '/') {
            result += '/';
        }
        return result;
    }

    int lstat(const char* path, struct stat* st) {
        return stat(path, st);
    }

    char* mkdtemp(char* path) {
        char* sep = ::strrchr(path, '/');
        if (sep) {
            struct stat st;
            int ret;
            *sep = '\0';  // temporarily zero-terminate the dirname.
            ret = ::stat(path, &st);
            *sep = '/';   // restore full path.
            if (ret < 0) {
                return NULL;
            }
            if (!S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                return NULL;
            }
        }

        // Loop. On each iteration, replace the XXXXXX suffix with a random
        // number.
        char* path_end = path + ::strlen(path);
        const size_t kSuffixLen = 6U;
        for (int tries = 128; tries > 0; tries--) {
            int random = rand() % 1000000;

            snprintf(path_end - kSuffixLen, kSuffixLen + 1, "%0d", random);
            if (mkdir(path) == 0) {
                return path;  // Success
            }
            if (errno != EEXIST) {
                return NULL;
            }
        }
        return NULL;
    }
#endif

    String mPath;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_TESTING_TEST_TEMP_DIR_H
