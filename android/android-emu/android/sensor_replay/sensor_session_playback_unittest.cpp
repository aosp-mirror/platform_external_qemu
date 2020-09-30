// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/sensor_replay/sensor_session_playback.h"

#include "android/base/files/PathUtils.h"

#include <gtest/gtest.h>

using namespace android;
using namespace sensorsessionplayback;

using android::base::PathUtils;
using android::base::StringView;
using android::base::System;

class SensorSessionPlaybackTest : public ::testing::Test {
protected:
    static std::unique_ptr<SensorSessionPlayback> sensorSessionPlayback;

    static void SetUpTestSuite() {
        sensorSessionPlayback.reset(new SensorSessionPlayback());

        // Load the test proto
        std::string path =
                PathUtils::join(System::get()->getProgramDirectory(),
                                "testdata", "sensor_proto_with_location");
        sensorSessionPlayback->LoadFrom(path);
    }

    static void TearDownTestSuite() { sensorSessionPlayback.reset(nullptr); }
};

std::unique_ptr<SensorSessionPlayback>
        SensorSessionPlaybackTest::sensorSessionPlayback;

static void checkLocationsRecord(emulator::SensorSession::SensorRecord record) {
    // There is only one sensor record with location info in this test proto.
    // So hardcode the values here.
    EXPECT_EQ(record.locations_size(), 1);
    EXPECT_EQ(record.locations(0).accuracy_mm(), 39599);
    EXPECT_EQ(record.locations(0).altitude_m(), -22.799999237060547);
    EXPECT_EQ(record.locations(0).vertical_accuracy_meters(), 2);
    EXPECT_EQ(record.locations(0).point().lat_e7(), 374179327);
    EXPECT_EQ(record.locations(0).point().lng_e7(), 3074104962);
}

TEST_F(SensorSessionPlaybackTest, TestLoadProto) {
    // There are 685 events in the test proto
    EXPECT_EQ(sensorSessionPlayback->event_count(), 685);
    // The duration of the test proto is 3331143561ns
    EXPECT_EQ(sensorSessionPlayback->session_duration(), 3331143561);
}

TEST_F(SensorSessionPlaybackTest, TestPlayAndStop) {
    sensorSessionPlayback->RegisterCallback(
            [](emulator::SensorSession::SensorRecord record) {
                if (record.locations_size() > 0) {
                    checkLocationsRecord(record);
                    sensorSessionPlayback->StopReplay();
                }
            });
    sensorSessionPlayback->StartReplay();
}

TEST_F(SensorSessionPlaybackTest, TestSeekToTime) {
    // Find the first record with location field and check the content
    sensorSessionPlayback->SeekToTime(1086720829);
    emulator::SensorSession::SensorRecord record =
            sensorSessionPlayback->GetCurrentSensorData();
    checkLocationsRecord(record);
}
