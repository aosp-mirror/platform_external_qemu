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
#include "android/base/testing/TestTempDir.h"
#include "android/base/testing/Utils.h"
#include "android/protobuf/LoadSave.h"

#include <gtest/gtest.h>
#include <fstream>

using android::base::TestTempDir;

using namespace android;
using namespace sensorsessionplayback;

TEST(SensorSessionPlayback, ReadNoneexistantFile) {
    std::unique_ptr<SensorSessionPlayback> sensorSessionPlayback;
    sensorSessionPlayback.reset(new SensorSessionPlayback());

    EXPECT_EQ(false, sensorSessionPlayback->loadFrom("i_dont_exist.pb"));
}

TEST(SensorSessionPlayback, ReadEmptyFile) {
    std::unique_ptr<SensorSessionPlayback> sensorSessionPlayback;
    sensorSessionPlayback.reset(new SensorSessionPlayback());

    char text[] = "";

    TestTempDir myDir("sensor_session_playback_tests");
    ASSERT_TRUE(myDir.path());  // NULL if error during creation.
    std::string path = myDir.makeSubPath("sensor_session_playback.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    EXPECT_EQ(false, sensorSessionPlayback->loadFrom(path));
}

TEST(SensorSessionPlayback, ReadGarbageFile) {
    std::unique_ptr<SensorSessionPlayback> sensorSessionPlayback;
    sensorSessionPlayback.reset(new SensorSessionPlayback());

    char text[] =
            "Some random data\n"
            "That is not a valid protobuf\n";

    TestTempDir myDir("sensor_session_playback_tests");
    ASSERT_TRUE(myDir.path());  // NULL if error during creation.
    std::string path = myDir.makeSubPath("sensor_session_playback.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    EXPECT_EQ(false, sensorSessionPlayback->loadFrom(path));
}

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

TEST(SensorSessionPlayback, WriteReadFile) {
    // Create a sensor session protobuf in memory
    emulator::SensorSession testSensorSession;
    const int expectedCount = 500;
    int64_t timestampNs = 0;

    auto* metadata = testSensorSession.mutable_session_metadata();
    metadata->set_unix_timestamp_ms(timestampNs);

    // a sensor record with location
    emulator::SensorSession::SensorRecord* testSensorRecord1 =
            testSensorSession.add_sensor_records();
    testSensorRecord1->set_timestamp_ns(timestampNs);
    auto* testLocation = testSensorRecord1->add_locations();
    auto* testPoint = testLocation->mutable_point();
    testPoint->set_lat_e7(374179327);
    testPoint->set_lng_e7(3074104962);
    testLocation->set_accuracy_mm(39599);
    testLocation->set_altitude_m(-22.799999237060547);
    testLocation->set_vertical_accuracy_meters(2);

    // Add sensor records with only timestamp
    emulator::SensorSession::SensorRecord* testSensorRecord2;
    for (int count = 1; count < expectedCount; count++) {
        testSensorRecord2 = testSensorSession.add_sensor_records();
        testSensorRecord2->set_timestamp_ns(timestampNs++);
    }

    // Write to protobuf to disk
    TestTempDir myDir("sensor_session_playback_tests");
    ASSERT_TRUE(myDir.path());  // NULL if error during creation.
    std::string path = myDir.makeSubPath("sensor_session_playback.pb");
    std::ofstream outStream(path.c_str(), std::ofstream::binary);
    testSensorSession.SerializeToOstream(&outStream);
    outStream.close();

    EXPECT_TRUE(android::base::System::get()->pathExists(path));
    android::base::System::FileSize fileSize;
    android::base::System::get()->pathFileSize(path, &fileSize);
    EXPECT_TRUE(fileSize > 0);

    // Read the protobuf
    std::unique_ptr<SensorSessionPlayback> sensorSessionPlayback;
    sensorSessionPlayback.reset(new SensorSessionPlayback());

    EXPECT_EQ(true, sensorSessionPlayback->loadFrom(path));

    EXPECT_EQ(expectedCount, sensorSessionPlayback->eventCount());
    // The duration of the test proto is timestampNs
    EXPECT_EQ(timestampNs - 1, sensorSessionPlayback->sessionDuration());

    EXPECT_EQ(0, sensorSessionPlayback->metadata().unix_timestamp_ms());

    sensorSessionPlayback->seekToTime(1);
    checkLocationsRecord(sensorSessionPlayback->getCurrentSensorData());
}
