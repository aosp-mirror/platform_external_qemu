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

#include "android/car_sensor_replay/sensor_session_playback.h"

#include <fcntl.h>  // for O_RDONLY
#include <algorithm>

#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/utils/debug.h"  // for VERBOSE_PRINT
#include "google/protobuf/text_format.h"

using android::base::AutoLock;
using android::base::System;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

// Flags that to control the playback state
static constexpr uint8_t REPLAY_START = 1;
static constexpr uint8_t REPLAY_STOP = 2;
static constexpr uint8_t REPLAY_PAUSE = 2;

SensorSessionPlayback::SensorSessionPlayback()
    : playThread_([this]() { playSensor(); }) {}

SensorSessionPlayback::~SensorSessionPlayback() {
    StopReplay();
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::LoadFrom(
        std::string filename) {
    std::string proto;

    // Load the binary proto
    android::base::ScopedFd tempfileFd(open(filename.c_str(), O_RDONLY));

    bool file_status = android::readFileIntoString(tempfileFd.get(), &proto);
    if (!file_status) {
        return SensorSessionPlayback::PROTO_PARSE_FAIL;
    }

    // Load into separate memory so that the old session remains valid if
    // loading fails.
    emulator::SensorSession new_session;
    if (!new_session.ParseFromString(proto)) {
        return SensorSessionPlayback::PROTO_PARSE_FAIL;
    }

    // Sort the sensor records by time.
    sensorRecordsSort(new_session.mutable_sensor_records());

    AutoLock lock(carVhalReplaytLock_);
    session_ = new_session;
    current_elapsed_time_ = 0;
    current_record_iterator_ = session_.sensor_records().begin();

    // Skip the record with negative timestamp
    while (current_record_iterator_->timestamp_ns() < 0) {
        current_record_iterator_++;
    }

    current_sensor_aggregate_.Clear();
    return SensorSessionPlayback::OK;
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::SeekToTime(
        DurationNs time) {
    //   Durations are signed. Make sure the given time is positive.
    if (time < 0)
        return SensorSessionPlayback::SEEK_TIME_NEGATIVE;

    emulator::SensorSession::SensorRecord timestamped_record;
    timestamped_record.set_timestamp_ns(time);
    AutoLock lock(carVhalReplaytLock_);
    google::protobuf::RepeatedPtrField<
            const emulator::SensorSession::SensorRecord>::iterator
            new_record_iterator = std::lower_bound(
                    session_.sensor_records().begin(),
                    session_.sensor_records().end(), timestamped_record,
                    [](emulator::SensorSession::SensorRecord a,
                       emulator::SensorSession::SensorRecord b) {
                        return a.timestamp_ns() < b.timestamp_ns();
                    });
    current_elapsed_time_ = time;

    // If we're seeking to the same place, we're done.
    if (new_record_iterator == current_record_iterator_)
        return SensorSessionPlayback::OK;

    // If we're seeking forward, fast forward from our current position.
    if (new_record_iterator == session_.sensor_records().end() ||
        new_record_iterator->timestamp_ns() >
                current_record_iterator_->timestamp_ns()) {
        for (auto iter = current_record_iterator_; iter != new_record_iterator;
             ++iter) {
            current_sensor_aggregate_.MergeFrom(*iter);
        }
    } else {
        // Otherwise, reconstruct the current sensor record from the beginning.
        current_record_iterator_ = new_record_iterator;
        current_sensor_aggregate_.Clear();
        for (auto iter = session_.sensor_records().begin();
             iter != current_record_iterator_; ++iter) {
            current_sensor_aggregate_.MergeFrom(*iter);
        }
    }

    return SensorSessionPlayback::OK;
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::StartReplay(
        float multiplier) {
    if (multiplier < 0)
        return SensorSessionPlayback::MULTIPLIER_NEGATIVE;
    carVhalReplayMultiMsg_.trySend(multiplier);
    carVhalReplayMsg_.trySend(REPLAY_START);
    playThread_.start();
    return SensorSessionPlayback::OK;
}

void SensorSessionPlayback::playSensor() {
    float multiplier;
    int replay_state;
    carVhalReplayMultiMsg_.tryReceive(&multiplier);
    carVhalReplayMsg_.tryReceive(&replay_state);

    DurationNs current_start_time = getUnixTimeNs() - current_elapsed_time_;

    int sensorrecordcount = 0;

    // Need add another channel msg for stop and start
    while (current_record_iterator_ != session_.sensor_records().end() &&
           replay_state == REPLAY_START) {
        current_elapsed_time_ =
                (getUnixTimeNs() - current_start_time) * multiplier;
        // D ("in the whileloop %" PRIu64, current_elapsed_time_);
        // Keep playing data until we've caught up.
        while ((current_record_iterator_ != session_.sensor_records().end()) &&
               current_elapsed_time_ >
                       current_record_iterator_->timestamp_ns()) {
            // Update the sensor record.
            current_sensor_aggregate_.MergeFrom(*current_record_iterator_);
            // Issue the callback if it's registered.
            if (callback_)
                callback_(*current_record_iterator_);
            // D ("print sensor record count %d, time stamp %" PRIu64,
            // sensorrecordcount++, current_record_iterator_->timestamp_ns());
            // Advance to the next record
            current_record_iterator_++;
        }
        // Sleep until the next record is ready.
        if (current_record_iterator_ != session_.sensor_records().end()) {
            carVhalReplayCV_.timedWait(
                    &carVhalReplaytLock_,
                    (current_start_time +
                     current_record_iterator_->timestamp_ns() -
                     getUnixTimeNs()) /
                            1000);
        }

        carVhalReplayMsg_.tryReceive(&replay_state);
    }
    carVhalReplayMsg_.trySend(REPLAY_STOP);
}

void SensorSessionPlayback::StopReplay() {
    carVhalReplayMsg_.trySend(REPLAY_STOP);
    playThread_.wait();
}

SensorSessionPlayback::DurationNs SensorSessionPlayback::getUnixTimeNs() {
    return System::get()->getUnixTimeUs() * 1000;
}
