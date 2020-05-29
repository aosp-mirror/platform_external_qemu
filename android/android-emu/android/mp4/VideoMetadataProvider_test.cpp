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

#include "android/mp4/VideoMetadataProvider.h"

#include "offworld.pb.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <gtest/gtest.h>

using namespace android::mp4;

typedef ::offworld::FieldInfo::Type Type;

class VideoMetadataProviderTest : public testing::Test {
protected:
    AVPacket mPacket1, mPacket2;
    std::unique_ptr<VideoMetadataProvider> mProvider;

    virtual void SetUp() override {
        initializeDatasetInfo();
        mProvider = VideoMetadataProvider::create(mDatasetInfo);
        initializePackets();
    }

private:
    DatasetInfo mDatasetInfo;
    uint8_t mPacketData1[8] = {0, 202, 154, 59, 0, 0, 0, 0};  // 1000000000, 0
    uint8_t mPacketData2[8] = {1, 202, 154, 59, 1, 0, 0, 0};  // 1000000001, 1

    void initializeDatasetInfo() {
        auto metadataInfo = mDatasetInfo.mutable_video_metadata();
        metadataInfo->set_stream_index(1);
        auto packetInfo = metadataInfo->mutable_video_metadata_packet();
        auto timestampInfo = packetInfo->mutable_timestamp();
        timestampInfo->set_type(Type::FieldInfo_Type_INT_32);
        timestampInfo->set_offset(0);
        auto frameNumInfo = packetInfo->mutable_frame_number();
        frameNumInfo->set_type(Type::FieldInfo_Type_INT_32);
        frameNumInfo->set_offset(4);
    }

    void initializePackets() {
        av_init_packet(&mPacket1);
        mPacket1.stream_index = 1;
        av_packet_from_data(&mPacket1, &mPacketData1[0], 8);
        av_init_packet(&mPacket2);
        mPacket2.stream_index = 1;
        av_packet_from_data(&mPacket2, &mPacketData2[0], 8);
    }
};

TEST_F(VideoMetadataProviderTest, createGetMetadata) {
    // Check metadata creation
    EXPECT_EQ(mProvider->createVideoMetadata(&mPacket1), 0);
    EXPECT_EQ(mProvider->createVideoMetadata(&mPacket2), 0);

    // Check the first frame's metadata
    EXPECT_TRUE(mProvider->hasNextFrameMetadata());
    const VideoMetadata metadata1 = mProvider->getNextFrameMetadata();
    EXPECT_EQ(metadata1.timestamp, 1000000000);
    EXPECT_EQ(metadata1.frameNumber, 0);

    // Check the second frame's metadata
    EXPECT_TRUE(mProvider->hasNextFrameMetadata());
    const VideoMetadata metadata2 = mProvider->getNextFrameMetadata();
    EXPECT_EQ(metadata2.timestamp, 1000000001);
    EXPECT_EQ(metadata2.frameNumber, 1);

    // Check that there is no more metadata
    EXPECT_FALSE(mProvider->hasNextFrameMetadata());
}
