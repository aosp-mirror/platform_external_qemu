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

#pragma once

#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/base/synchronization/MessageChannel.h"     // for MessageC...
#include "android/base/threads/FunctorThread.h"              // for FunctorT...
#include "google/protobuf/repeated_field.h"
#include "sensor_session.pb.h"


class SensorSessionPlayback {
    public:

        // There is no equivalent for absl/status/status.h in emulator code
        enum SensorSessionStatus { FAILED_PRECONDITION, OK, FAIL, PROTO_PARSE_OK, PROTO_PARSE_FAIL, SEEK_TIME_NEGATIVE, MULTIPLIER_NEGATIVE};

        typedef int64_t DurationNs;

        static SensorSessionPlayback Create() {
            return SensorSessionPlayback();
        }

        explicit SensorSessionPlayback();

        virtual ~SensorSessionPlayback();

        // Loads a sensor session file into memory. This method blocks if called while
        // playback is running.
        SensorSessionStatus LoadFrom(std::string filename);

        // Sets playback to begin at a specified point in time. Given duration is
        // relative to session start time.
        SensorSessionStatus SeekToTime(DurationNs time);

        // Returns the current sensor data.
        emulator::SensorSession::SensorRecord GetCurrentSensorData() const {
            return current_sensor_aggregate_;
        }

        // Register a callback to be run whenever a new event occurs.
        void RegisterCallback(std::function<void(emulator::SensorSession::SensorRecord)> fn) {
            callback_ = fn;
        }

        // Begins sensor playback.
        SensorSessionStatus StartReplay(float multiplier = 1);

        // Pauses sensor playback.
        void StopReplay();

        // Metadata accessors.
        emulator::SensorSession::SessionMetadata metadata() const {
            return session_.session_metadata();
        }

        DurationNs session_duration() const {
            return session_.sensor_records().empty() ? 0 :
                session_.sensor_records().rbegin()->timestamp_ns();
        }

        int event_count() const { return session_.sensor_records().size(); }

    private:
        // Currently loaded session data.
        emulator::SensorSession session_;

        // Tracks currently elapsed time during playback.
        DurationNs current_elapsed_time_ = 0;

        // Tracks current position in data during playback.
        google::protobuf::RepeatedPtrField<const emulator::SensorSession::SensorRecord>::iterator
            current_record_iterator_;

        // Collection of current sensor data.
        emulator::SensorSession::SensorRecord current_sensor_aggregate_;

        // Function to be called when data is available. Set with RegisterCallback().
        std::function<void(emulator::SensorSession::SensorRecord)> callback_;

        android::base::FunctorThread playThread_;
        android::base::ConditionVariable carVhalReplayCV_;
        android::base::Lock carVhalReplaytLock_;
        android::base::MessageChannel<float, 4> carVhalReplayMsg_;

        static bool sensor_comparator(const emulator::SensorSession::SensorRecord* z, const emulator::SensorSession::SensorRecord* y) {
            return z->timestamp_ns() < y->timestamp_ns();
        }

        void playSensor();

        inline void sensorRecordsSort(google::protobuf::RepeatedPtrField<emulator::SensorSession::SensorRecord>* array) {
            std::sort(array->pointer_begin(), array->pointer_end(), sensor_comparator);
        }

        // SensorSessionPlayback is neither copyable nor movable.
        SensorSessionPlayback(const SensorSessionPlayback&) = delete;
        SensorSessionPlayback& operator=(const SensorSessionPlayback&) = delete;
};
