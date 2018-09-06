// Copyright 2018 The Android Open Source Project
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
#include <gtest/gtest.h>

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

using android::base::PathUtils;
using android::base::System;

#define EMU_BINARY_BASENAME "emulator"

#ifdef _WIN32
#define EMU_BINARY_SUFFIX ".exe"
#else
#define EMU_BINARY_SUFFIX ""
#endif

static const int kExecTimeoutMs = 5000;

TEST(HelloEmulator, BasicAccelCheck) {
    auto dir = android::base::System::get()->getProgramDirectory();
    std::string emulatorBinaryFilename(EMU_BINARY_BASENAME EMU_BINARY_SUFFIX);
    auto emuLauncherPath = PathUtils::join(dir, emulatorBinaryFilename);
    EXPECT_TRUE(path_exists(emuLauncherPath.c_str()));

    std::string result =
        System::get()->runCommandWithResult(
            {emuLauncherPath, "-accel-check"},
            5000).valueOr({});

    EXPECT_FALSE(result == "");
}
