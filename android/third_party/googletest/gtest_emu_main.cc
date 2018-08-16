// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/system/System.h"

#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using android::base::System;

int main(int argc, char** argv) {
    std::string dir = System::get()->getProgramDirectory();
    if (dir.empty()) {
        std::cerr << "Failed to get the program directory." << std::endl;
        return -1;
    }

    if (!System::get()->setCurrentDirectory(dir)) {
        std::cerr << "Failed to set the current directory to " << dir
                  << std::endl;
        return -2;
    }

    // InitGoogleMock will automatically initialize GoogleTest as well.
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
