// Copyright (C) 2019 The Android Open Source Project
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

#include "android/mp4/MP4Dataset.h"

#include <gtest/gtest.h>

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

using namespace android::mp4;

class Mp4DatasetTest : public testing::Test {
protected:
    std::unique_ptr<Mp4Dataset> mDataset;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        mDataset = Mp4Dataset::create(absDataPath);
    }
};

TEST_F(Mp4DatasetTest, init) {
    EXPECT_EQ(mDataset->getFormatContext()->nb_streams, 5);
    EXPECT_EQ(mDataset->getAudioStreamIndex(), -1);
    EXPECT_EQ(mDataset->getVideoStreamIndex(), 0);
}