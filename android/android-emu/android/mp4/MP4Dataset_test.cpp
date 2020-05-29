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

#include <gtest/gtest.h>                         // for EXPECT_EQ, Message
#include <libavformat/avformat.h>                // for AVFormatContext

#include "android/base/files/PathUtils.h"        // for PathUtils
#include "android/base/system/System.h"          // for System
#include "android/hw-sensors.h"                  // for ANDROID_SENSOR_ACCEL...
#include "offworld.pb.h"  // for DataStreamInfo

using namespace android::mp4;

class Mp4DatasetTest : public testing::Test {
protected:
    std::unique_ptr<Mp4Dataset> mDataset;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        initializeDatasetInfo();

        mDataset = Mp4Dataset::create(absDataPath, mDatasetInfo);
    }

private:
    DatasetInfo mDatasetInfo;

    void initializeDatasetInfo() {
        auto accelInfo = mDatasetInfo.mutable_accelerometer();
        accelInfo->set_stream_index(1);
        auto gyroInfo = mDatasetInfo.mutable_gyroscope();
        gyroInfo->set_stream_index(2);
        auto magInfo = mDatasetInfo.mutable_magnetic_field();
        magInfo->set_stream_index(3);
    }
};

TEST_F(Mp4DatasetTest, init) {
    EXPECT_EQ(mDataset->getFormatContext()->nb_streams, 5);
    EXPECT_EQ(mDataset->getAudioStreamIndex(), -1);
    EXPECT_EQ(mDataset->getVideoStreamIndex(), 0);
    EXPECT_EQ(mDataset->getSensorDataStreamIndex(ANDROID_SENSOR_ACCELERATION),
              1);
    EXPECT_EQ(mDataset->getSensorDataStreamIndex(ANDROID_SENSOR_GYROSCOPE), 2);
    EXPECT_EQ(mDataset->getSensorDataStreamIndex(ANDROID_SENSOR_MAGNETIC_FIELD),
              3);
}
