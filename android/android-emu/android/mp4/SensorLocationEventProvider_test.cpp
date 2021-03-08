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

#include "android/mp4/SensorLocationEventProvider.h"

#include "offworld.pb.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <gtest/gtest.h>

using namespace android::mp4;
using namespace android::automation;

typedef ::offworld::FieldInfo::Type Type;

class SensorLocationEventProviderTest : public testing::Test {
protected:
    AVPacket mPacket1, mPacket2;
    std::shared_ptr<SensorLocationEventProvider> mProvider;

    virtual void SetUp() override {
        initializeDatasetInfo();
        mProvider = SensorLocationEventProvider::create(mDatasetInfo);
        initializePackets();
        mProvider->startFromTimestamp(1000000000);
    }

    virtual void TearDown() override { mProvider->stop(); }

private:
    DatasetInfo mDatasetInfo;
    uint8_t mPacketData1[8] = {0,  202, 154, 59,
                               44, 1,   0,   0};  // 1000000000, 300
    uint8_t mPacketData2[8] = {1,   202, 154, 59,
                               144, 1,   0,   0};  // 1000000001, 400

    void initializeDatasetInfo() {
        auto accelInfo = mDatasetInfo.mutable_accelerometer();
        accelInfo->set_stream_index(2);
        auto packetInfo = accelInfo->mutable_sensor_packet();
        auto timestampInfo = packetInfo->mutable_timestamp();
        timestampInfo->set_type(Type::FieldInfo_Type_INT_32);
        timestampInfo->set_offset(0);
        auto valueInfo = packetInfo->add_value();
        valueInfo->set_type(Type::FieldInfo_Type_INT_32);
        valueInfo->set_offset(4);
    }

    void initializePackets() {
        av_init_packet(&mPacket1);
        mPacket1.stream_index = 2;
        av_packet_from_data(&mPacket1, &mPacketData1[0], 8);
        av_init_packet(&mPacket2);
        mPacket2.stream_index = 2;
        av_packet_from_data(&mPacket2, &mPacketData2[0], 8);
    }
};

TEST_F(SensorLocationEventProviderTest, createConsumeEvents) {
    // Check event creation
    EXPECT_EQ(mProvider->createEvent(&mPacket1), 0);
    EXPECT_EQ(mProvider->createEvent(&mPacket2), 0);

    // Check event1 consumption
    DurationNs delay;
    EXPECT_EQ(mProvider->getNextCommandDelay(&delay), NextCommandStatus::OK);
    EXPECT_EQ(delay, 0);
    const emulator_automation::RecordedEvent event1 =
            mProvider->consumeNextCommand();
    EXPECT_EQ(event1.delay(), 1000000000);
    EXPECT_TRUE(event1.has_sensor_override());
    EXPECT_EQ(event1.sensor_override().value().data_size(), 1);
    EXPECT_EQ(event1.sensor_override().value().data(0),
              static_cast<float>(300));

    // Check event2 consumption
    EXPECT_EQ(mProvider->getNextCommandDelay(&delay), NextCommandStatus::OK);
    EXPECT_EQ(delay, 1);
    const emulator_automation::RecordedEvent event2 =
            mProvider->consumeNextCommand();
    EXPECT_EQ(event2.delay(), 1000000001);
    EXPECT_TRUE(event2.has_sensor_override());
    EXPECT_EQ(event2.sensor_override().value().data_size(), 1);
    EXPECT_EQ(event2.sensor_override().value().data(0),
              static_cast<float>(400));

    // Check that there is no more events
    EXPECT_EQ(mProvider->getNextCommandDelay(&delay), NextCommandStatus::NONE);
}
