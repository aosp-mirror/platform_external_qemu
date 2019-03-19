/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/videoplayback/videoplayback-utils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

using namespace android::videoplayback;
using testing::Eq;

TEST(VideoplaybackUtils, parsesHeader) {
    std::string testBuffer = "P6 1280 720 255 irrelevantdata";
    PPMInfo info;
    int length = parsePPMHeader(&info, (const unsigned char*)testBuffer.c_str(),
                                testBuffer.size());

    EXPECT_THAT(length, Eq(16));
    EXPECT_THAT(info.width, Eq(1280));
    EXPECT_THAT(info.height, Eq(720));
    EXPECT_THAT(info.maxColor, Eq(255));
}

TEST(VideoplaybackUtils, checksMagicNumber) {
  std::string testBuffer = "R2 1280 720 255 irrelevantdata";
  PPMInfo info;

  int length = parsePPMHeader(&info, (const unsigned char*)testBuffer.c_str(),
                              testBuffer.size());

  EXPECT_THAT(length, Eq(-1));
}

TEST(VideoplaybackUtils, checksNumberOfValuesInBuffer) {
  std::string testBuffer = "P6 1280 720 255";
  PPMInfo info;

  int length = parsePPMHeader(&info, (const unsigned char*)testBuffer.c_str(),
                              testBuffer.size());

  EXPECT_THAT(length, Eq(-1));
}
