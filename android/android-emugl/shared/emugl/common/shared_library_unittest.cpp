// Copyright (C) 2014 The Android Open Source Project
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

#include "emugl/common/shared_library.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <string>

#include <limits.h>
#include <string.h>

using android::base::PathUtils;
using android::base::System;

namespace emugl {

namespace {

// Return the name/path of the test shared library to load.
// Note that this doesn't include a platform-specific extension.
// This assumes that the test shared library is under the lib/ sub-directory
// of the current executable's path!
std::string GetTestLibraryName() {
#ifdef __x86_64__
    static const char kSubDir[] = "lib64";
#else
    static const char kSubDir[] = "lib";
#endif
    static const char kTestLibraryBasename[] = "libemugl_test_shared_library";

    std::string path =
            PathUtils::join(System::get()->getProgramDirectory(), kSubDir,
                            kTestLibraryBasename);
    printf("Library path: %s\n", path.c_str());
    return path;
}

class SharedLibraryTest : public testing::Test {
public:
    SharedLibraryTest() {
        // Locate the shared library
        mLibraryPath = GetTestLibraryName();
    }

    ~SharedLibraryTest() {}

    const char* library_path() const { return mLibraryPath.c_str(); }

private:
    std::string mLibraryPath;
};

class ScopedSharedLibrary {
public:
    explicit ScopedSharedLibrary(const SharedLibrary* lib) : mLib(lib) {}
    ~ScopedSharedLibrary() {
    }
    const SharedLibrary* get() const { return mLib; }

    const SharedLibrary* operator->() { return mLib; }

    void release() {
        mLib = NULL;
    }

private:
    const SharedLibrary* mLib;
};

}  // namespace

TEST_F(SharedLibraryTest, Open) {
    ScopedSharedLibrary lib(SharedLibrary::open(library_path()));
    EXPECT_TRUE(lib.get());
}

TEST_F(SharedLibraryTest, OpenFailureWithError) {
    char error[256];
    error[0] = '\0';
    SharedLibrary* lib = SharedLibrary::open("/tmp/does/not/exists",
                                             error,
                                             sizeof(error));
    EXPECT_FALSE(lib);
    EXPECT_TRUE(error[0])
            << "Could not get error string when failing to load library";
    printf("Expected library load failure: [%s]\n", error);
}

TEST_F(SharedLibraryTest, OpenLibraryWithExtension) {
    std::string path = library_path();

    // test extension append
    ScopedSharedLibrary libNoExtension(SharedLibrary::open(path.c_str()));
    EXPECT_TRUE(libNoExtension.get());
    libNoExtension.release();

#ifdef _WIN32
    path += ".dll";
#elif defined(__APPLE__)
    // try to open the library without an extension

    path += ".dylib";
#else
    path += ".so";
#endif

    // test open with prepended extension
    ScopedSharedLibrary lib(SharedLibrary::open(path.c_str()));
    EXPECT_TRUE(lib.get());
}

#ifdef __APPLE__
TEST_F(SharedLibraryTest, OpenLibraryWithoutExtension) {
    const char* library = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
    ScopedSharedLibrary lib(SharedLibrary::open(library));
    EXPECT_TRUE(lib.get());
}
#endif

TEST_F(SharedLibraryTest, FindSymbol) {
    ScopedSharedLibrary lib(SharedLibrary::open(library_path()));
    EXPECT_TRUE(lib.get());

    if (lib.get()) {
        typedef int (*FooFunction)(void);

        FooFunction foo_func = reinterpret_cast<FooFunction>(
                lib->findSymbol("foo_function"));
        EXPECT_TRUE(foo_func);
        EXPECT_EQ(42, (*foo_func)());
    }
}

}  // namespace emugl
