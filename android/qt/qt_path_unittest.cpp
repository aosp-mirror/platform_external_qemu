// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/qt/qt_path.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(androidQtGetLibraryDir, Qt32) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib\\qt";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib/qt";
#endif

    TestSystem testSys(basePath, 32);

    EXPECT_STREQ(resultPath, androidQtGetLibraryDir().c_str());
}

TEST(androidQtGetLibraryDir, Qt64) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib64\\qt";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib64/qt";
#endif

    TestSystem testSys(basePath, 64);

    EXPECT_STREQ(resultPath, androidQtGetLibraryDir().c_str());
}
