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

#include <fcntl.h>

#include <algorithm>

#include "android/base/files/ScopedFd.h"
#include "android/protobuf/LoadSave.h"
#include "google/protobuf/text_format.h"

using android::protobuf::loadProtobuf;
using android::protobuf::ProtobufLoadResult;

namespace android {
namespace sensorsessionplayback {

using android::base::AutoLock;
using android::base::System;
using android::sensorsessionplayback::SensorSessionPlayback;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

// Flags that to control the playback state
static constexpr uint8_t REPLAY_START = 1;
static constexpr uint8_t REPLAY_STOP = 2;

static constexpr int64_t US_TO_NS = 1000;

SensorSessionPlayback::SensorSessionPlayback() {}

SensorSessionPlayback::~SensorSessionPlayback() {
    StopReplay();
}

// Load the binary proto
bool SensorSessionPlayback::LoadFrom(std::string filename) {
    // Load into separate memory so that the old session remains valid if
    // loading fails.
    emulator::SensorSession new_session;
    uint64_t size = 0;

    ProtobufLoadResult loadResult = loadProtobuf(filename, new_session, &size);

    if (loadResult != ProtobufLoadResult::Success)
    {
        return false;
    }

    // Sort the sensor records by time.
    sensorRecordsSort(new_session.mutable_sensor_records());

    AutoLock lock(carVhalReplaytLock_);
    session_ = new_session;

    resetSession();

    return true;
}

void SensorSessionPlayback::resetSession() {
    current_elapsed_time_ = 0;
    current_record_iterator_ = session_.sensor_records().begin();

    current_sensor_aggregate_.Clear();
}

std::vector<int> SensorSessionPlayback::GetSensorRecordTimeLine(
        DurationNs interval) {
    std::vector<int> recordCounts;
    DurationNs current_threshold = interval;
    DurationNs threshold_interval = interval;

    int count = 0;
    for (auto it = session_.sensor_records().begin();
         it != session_.sensor_records().end(); it++) {
        count++;
        if (it->timestamp_ns() > current_threshold) {
            recordCounts.push_back(count);
            count = 0;
            current_threshold += threshold_interval;
        }
    }
    return recordCounts;
}

bool SensorSessionPlayback::SeekToTime(DurationNs time) {
    //  Durations are signed. Make sure the given time is positive.
    if (time < 0)
        return false;

    emulator::SensorSession::SensorRecord timestamped_record;
    timestamped_record.set_timestamp_ns(time);

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
        return true;

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

    return true;
}

bool SensorSessionPlayback::StartReplay(float multiplier) {
    if (multiplier < 0) {
        return false;
    }
    carVhalReplayMultiMsg_.trySend(multiplier);
    carVhalReplayMsg_.trySend(REPLAY_START);
    playThread_.reset(
            new android::base::FunctorThread([this]() { playSensor(); }));

    playThread_->start();
    return true;
}

void SensorSessionPlayback::playSensor() {
    float multiplier;
    int replay_state;
    carVhalReplayMultiMsg_.tryReceive(&multiplier);
    carVhalReplayMsg_.tryReceive(&replay_state);

    DurationNs current_start_time = getUnixTimeNs() - current_elapsed_time_;

    // Need add another channel msg for stop and start
    while (current_record_iterator_ != session_.sensor_records().end() &&
           replay_state == REPLAY_START) {
        current_elapsed_time_ =
                (getUnixTimeNs() - current_start_time) * multiplier;

        // Keep playing data until we've caught up.
        while ((current_record_iterator_ != session_.sensor_records().end()) &&
               current_elapsed_time_ >
                       current_record_iterator_->timestamp_ns()) {
            // Update the sensor record.
            current_sensor_aggregate_.MergeFrom(*current_record_iterator_);
            // Issue the callback if it's registered.
            if (callback_)
                callback_(*current_record_iterator_);
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
                            US_TO_NS);
        }

        carVhalReplayMsg_.tryReceive(&replay_state);
    }
}

void SensorSessionPlayback::StopReplay() {
    carVhalReplayMsg_.trySend(REPLAY_STOP);
    if (playThread_) {
        playThread_->wait();
    }
}

SensorSessionPlayback::DurationNs SensorSessionPlayback::getUnixTimeNs() {
    return System::get()->getUnixTimeUs() * US_TO_NS;
}

}  // namespace sensorsessionplayback
}  // namespace android
