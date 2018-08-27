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

#include "android/base/AlignedBuf.h"

#include "render_api_platform_types.h"

#include <GLES3/gl31.h>

#include <gtest/gtest.h>

namespace emugl {

using TestTexture = android::AlignedBuf<uint8_t, 4>;

testing::AssertionResult RowMatches(int rowIndex, size_t rowBytes,
                                    unsigned char* expected, unsigned char* actual);

testing::AssertionResult ImageMatches(int width, int height, int bpp, int rowLength,
                                      unsigned char* expected, unsigned char* actual);


FBNativeWindowType createTestNativeWindow(int x, int y, int width, int height, int dpr);

// Creates an asymmetric test pattern with various formats.
TestTexture createTestPatternRGB888(int width, int height);
TestTexture createTestPatternRGBA8888(int width, int height);

// Creates a test pattern of the specified color.
TestTexture createTestTextureRGB888SingleColor(int width, int height, float r, float g, float b);
TestTexture createTestTextureRGBA8888SingleColor(int width, int height, float r, float g, float b, float a);

// Return the name associated with |v| as a string.
const char* getEnumString(GLenum v);

} // namespace emugl
