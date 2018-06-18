// Copyright (C) 2018 The Android Open Source Project
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
#pragma once

#include <gtest/gtest.h>

#include "render_api_platform_types.h"

namespace gltest {

testing::AssertionResult RowMatches(int rowIndex, size_t rowBytes,
                                    unsigned char* expected, unsigned char* actual);

testing::AssertionResult ImageMatches(int width, int height, int bpp, int rowLength,
                                      unsigned char* expected, unsigned char* actual);


FBNativeWindowType createTestNativeWindow(int x, int y, int width, int height, int dpr);

} // namespace gltest
