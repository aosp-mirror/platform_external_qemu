/* Copyright (C) 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

///
// sensor_session_playback.h
//
// SensorSessionPlayback provides apis to load/replay the sensor files generated
// by sensor record apk. Sensor files are proto binary, proto is defined in
// android/android-emu/android/emulation/proto/sensor_session.proto
// The sensor records are not sorted in the proto binary, they will be sorted in
// LoadFrom function.
//
// Human readable example:
//
// session_metadata {
//   unix_timestamp_ms: 1599769231274
//   version {
//     major: 0
//     minor: 1
//     patch: 0
//   }
//   name: "Thu Sep 10 20:20:31 GMT 2020"
//   sensors: SENSOR_TYPE_PLATFORM_LOCATION
//   sensors: SENSOR_TYPE_CAR_PROPERTY
//   sensors: SENSOR_TYPE_SENSOR_EVENT
//   sensors: SENSOR_TYPE_FUSED_LOCATION
//   car_property_configs {
//     key: 356516106
//     value {
//       access: 1
//       area_type: 3
//       change_mode: 0
//       max_sample_rate: 0
//       min_sample_rate: 0
//       supported_areas {
//         area_id: 0
//         area_config {
//           min_int32_value: 0
//           max_int32_value: 0
//         }
//       }
//       type: "java.lang.Integer"
//     }
//   }
//   subscribed_sensors {
//     key: "android.sensor.accelerometer_Invensense Accelerometer"
//     value {
//       name: "Invensense Accelerometer"
//       vendor: "Invensense"
//       version: 1
//       type: 1
//       max_range: 39.2266
//       resolution: 0.0011971008
//       power_ma: 0.5
//       min_delay_us: 2000
//       fifo_reserved_event_count: 0
//       fifo_max_event_count: 0
//       string_type: "android.sensor.accelerometer"
//       max_delay_us: 250000
//       id: 0
//       reporting_mode: 0
//       highest_direct_report_rate_level: 0
//       direct_channel_memory_file_supported: false
//       direct_channel_hardware_buffer_supported: false
//       wake_up_sensor: false
//       dynamic_sensor: false
//       additional_info_supported: false
//     }
//   }
// sensor_records {
//   timestamp_ns: 101682689
//   sensor_events {
//     key: "android.sensor.gyroscope_uncalibrated_Invensense Gyroscope"
//     value {
//       values: 0.010652378
//       values: -0.025566613
//       values: -0.005326588
//       values: 0.011530399
//       values: -0.025104554
//       values: -0.005609416
//       accuracy: 3
//     }
//   }
// }
//
// example use:
//
//    // Load proto
//    sensorSessionPlayback->LoadFrom(path);
//
//    // Register the callback
//    sensorSessionPlayback->RegisterCallback(
//              [](emulator::SensorSession::SensorRecord record) {
//                  // Add user's desired responses here
//              });
//
//    // Start the playback
//    sensorSessionPlayback->StartReplay();

//    // Stop the playback
//    sensorSessionPlayback->StopReplay();
//
//    // Move the start point to a specific time
//    sensorSessionPlayback->SeekToTime(timestamp);
//
// See sensor_session_playback_unittest.cpp

#pragma once

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION

#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/base/synchronization/MessageChannel.h"     // for MessageC...
#include "android/base/threads/FunctorThread.h"              // for FunctorT...
#include "google/protobuf/repeated_field.h"

#include "sensor_session.pb.h"

namespace android {
namespace sensorsessionplayback {

class SensorSessionPlayback {
public:
    typedef int64_t DurationNs;

    explicit SensorSessionPlayback();

    virtual ~SensorSessionPlayback();

    // Loads a sensor session file into memory. This method blocks if called
    // while playback is running.
    bool loadFrom(std::string filename);

    // Sets playback to begin at a specified point in time. Given duration is
    // relative to session start time.
    bool seekToTime(DurationNs time);

    // Returns the current sensor data.
    emulator::SensorSession::SensorRecord getCurrentSensorData() const {
        return mCurrentSensorAggregate;
    }

    // Register a callback to be run whenever a new event occurs.
    void registerCallback(
            std::function<void(emulator::SensorSession::SensorRecord)> fn) {
        mCallback = fn;
    }

    // Begins sensor playback.
    bool startReplay(float multiplier = 1);

    // Pauses sensor playback.
    void stopReplay();

    // Metadata accessors.
    inline emulator::SensorSession::SessionMetadata metadata() const {
        return mSession.session_metadata();
    }

    // Return the duration of current session
    inline DurationNs sessionDuration() const {
        return mSession.sensor_records().empty()
                       ? 0
                       : mSession.sensor_records().rbegin()->timestamp_ns();
    }

    // Return the event count of current session
    inline int eventCount() const { return mSession.sensor_records().size(); }

    // Split the whole session by internal
    // Return the event count in each interval
    std::vector<int> getSensorRecordTimeLine(DurationNs interval);

private:
    // The main playback function used in playback thread
    void playSensor();

    void sensorRecordsSort(google::protobuf::RepeatedPtrField<
                           emulator::SensorSession::SensorRecord>* array) {
        std::sort(array->pointer_begin(), array->pointer_end(),
                  sensorComparator);
    }

    // Return current system time in ns
    DurationNs getUnixTimeNs();

    static bool sensorComparator(
            const emulator::SensorSession::SensorRecord* z,
            const emulator::SensorSession::SensorRecord* y) {
        return z->timestamp_ns() < y->timestamp_ns();
    }

    // Currently loaded session data. Need lock protection.
    emulator::SensorSession mSession;

    // Tracks currently elapsed time during playback.
    DurationNs mCurrentElapsedTime = 0;

    // Tracks current position in data during playback.
    google::protobuf::RepeatedPtrField<
            const emulator::SensorSession::SensorRecord>::iterator
            mCurrentRecordIterator;

    // Collection of current sensor data.
    emulator::SensorSession::SensorRecord mCurrentSensorAggregate;

    // Function to be called when data is available. Set with
    // RegisterCallback().
    std::function<void(emulator::SensorSession::SensorRecord)> mCallback;

    // SensorSessionPlayback is neither copyable nor movable.
    SensorSessionPlayback(const SensorSessionPlayback&) = delete;
    SensorSessionPlayback& operator=(const SensorSessionPlayback&) = delete;

    // Playback thread
    std::unique_ptr<base::FunctorThread> mPlayThread;
    base::ConditionVariable mCarVhalReplayCV;
    base::Lock mCarVhalReplayLock;
    base::MessageChannel<float, 4> mCarVhalReplayMultiMsg;
    base::MessageChannel<int, 2> mCarVhalReplayMsg;
};

}  // namespace sensorsessionplayback
}  // namespace android
