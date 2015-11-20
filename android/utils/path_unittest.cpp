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

#include "android/utils/path.h"
#include "gtest/gtest.h"

namespace android {
namespace path {

TEST(Path, EscapePath) {
    const char linuxInputPath[]    = "/Linux/style_with/various,special==character%s";
    const char linuxOutputPath[]   = "/Linux/style_with/various%Cspecial%E%Echaracter%Ps";

    const char windowsInputPath[]  = "C:\\Windows\\style:=with,,other%\\types of characters";
    const char windowsOutputPath[] = "C:\\Windows\\style:%Ewith%C%Cother%P\\types of characters";

    char *result;

    // Escape Linux style paths
    result = path_escape_path(linuxInputPath);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(linuxOutputPath, result);

    path_unescape_path(result);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(linuxInputPath, result);

    free(result);

    // Escape Windows style paths
    result = path_escape_path(windowsInputPath);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(windowsOutputPath, result);

    path_unescape_path(result);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(windowsInputPath, result);

    free(result);
}

}  // namespace path
}  // namespace android
