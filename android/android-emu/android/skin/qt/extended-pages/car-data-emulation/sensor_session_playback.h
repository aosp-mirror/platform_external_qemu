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

#include <QTimer>      // for QTimer

#include "sensor_session.pb.h"
// #include "net/proto2/public/repeated_field.h"
// #include "third_party/absl/status/status.h"
// #include "third_party/absl/time/time.h"
// #include "thread/fiber/fiber.h"
// #include "util/time/clock.h"


class SensorSessionPlayback {
    public:
        static SensorSessionPlayback Create() {
            return SensorSessionPlayback(util::Clock::RealClock());
        }

        explicit SensorSessionPlayback(util::Clock* clock) : clock_(clock) {}
        virtual ~SensorSessionPlayback() { StopReplay(); }

        // Loads a sensor session file into memory. This method blocks if called while
        // playback is running.
        absl::Status LoadFrom(std::string filename);

        // Sets playback to begin at a specified point in time. Given duration is
        // relative to session start time.
        absl::Status SeekToTime(absl::Duration time);

        // Returns the current sensor data.
        SensorSession::SensorRecord GetCurrentSensorData() const {
            return current_sensor_aggregate_;
        }

        // Register a callback to be run whenever a new event occurs.
        void RegisterCallback(std::function<void(SensorSession::SensorRecord)> fn) {
            callback_ = fn;
        }

        // Begins sensor playback.
        absl::Status StartReplay(float multiplier = 1);

        // Pauses sensor playback.
        void StopReplay();

        // Metadata accessors.
        SensorSession::SessionMetadata metadata() const {
            return session_.session_metadata();
        }

        absl::Duration session_duration() const {
            return absl::Nanoseconds(session_.sensor_records().empty() ? 0 :
                session_.sensor_records().rbegin()->timestamp_ns());
        }

        int event_count() const { return session_.sensor_records().size(); }

    // private:
    //     // Exposes private members for testing, if necessary.
    //     friend class SensorSessionPlaybackPeer;

    //     // Currently loaded session data.
    //     SensorSession session_;

    //     // Source of timing information.
    //     util::Clock* clock_;

    //     // Tracks currently elapsed time during playback.
    //     absl::Duration current_elapsed_time_ = absl::ZeroDuration();

    //     // Tracks current position in data during playback.
    //     proto2::RepeatedPtrField<const SensorSession::SensorRecord>::iterator
    //         current_record_iterator_;

    //     // Collection of current sensor data.
    //     SensorSession::SensorRecord current_sensor_aggregate_;

    //     // Function to be called when data is available. Set with RegisterCallback().
    //     std::function<void(SensorSession::SensorRecord)> callback_;

    //     // Threading structures.
    //     std::unique_ptr<thread::Fiber> fiber_;
    //     absl::Mutex mutex_;

    //     // SensorSessionPlayback is neither copyable nor movable.
    //     SensorSessionPlayback(const SensorSessionPlayback&) = delete;
    //     SensorSessionPlayback& operator=(const SensorSessionPlayback&) = delete;
};
