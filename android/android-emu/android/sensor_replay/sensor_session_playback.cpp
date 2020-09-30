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
#include "android/base/misc/FileUtils.h"
#include "google/protobuf/text_format.h"

namespace android {
namespace sensorsessionplayback {

using android::base::AutoLock;
using android::base::System;
using android::sensorsessionplayback::SensorSessionPlayback;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

// Flags that to control the playback state
static constexpr uint8_t REPLAY_START = 1;
static constexpr uint8_t REPLAY_STOP = 2;

SensorSessionPlayback::SensorSessionPlayback() {}

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

    resetSession();

    return SensorSessionPlayback::OK;
}

void SensorSessionPlayback::resetSession() {
    current_elapsed_time_ = 0;
    current_record_iterator_ = session_.sensor_records().begin();

    // Skip the records with negative timestamp
    while (current_record_iterator_->timestamp_ns() < 0) {
        current_record_iterator_++;
    }

    current_sensor_aggregate_.Clear();
}

std::string SensorSessionPlayback::app_version() const {
    if (session_.has_session_metadata() &&
        session_.session_metadata().has_version()) {
        emulator::SensorSession::SessionMetadata::Version version =
                session_.session_metadata().version();
        std::string versio_string =
                version.has_major() ? std::to_string(version.major()) : "0";
        versio_string.append(".").append(
                version.has_minor() ? std::to_string(version.minor()) : "0");
        versio_string.append(".").append(
                version.has_patch() ? std::to_string(version.patch()) : "0");
        return versio_string;
    }

    return "N/A";
}

const std::vector<std::string> SensorSessionPlayback::sensor_list() {
    std::vector<std::string> sensor_list;
    auto subscribed_sensors = session_.session_metadata().subscribed_sensors();
    for (auto it = subscribed_sensors.begin(); it != subscribed_sensors.end();
         it++) {
        sensor_list.push_back(it->first);
    }

    const std::vector<std::string> sensor_list_const = sensor_list;
    return sensor_list_const;
}

const std::vector<int> SensorSessionPlayback::car_property_id_list() {
    std::vector<int> car_property_id_list;
    auto car_property_configs =
            session_.session_metadata().car_property_configs();
    for (auto it = car_property_configs.begin();
         it != car_property_configs.end(); it++) {
        car_property_id_list.push_back(it->first);
    }

    const std::vector<int> car_property_id_list_const = car_property_id_list;
    return car_property_id_list_const;
}

void SensorSessionPlayback::getSensorRecordTimeLine(
        std::vector<int>* recordCounts,
        DurationNs interval) {
    DurationNs current_threshold = interval;
    DurationNs threshold_interval = interval;

    int count = 0;
    for (auto it = session_.sensor_records().begin();
         it != session_.sensor_records().end(); it++) {
        count++;
        if (it->timestamp_ns() > current_threshold) {
            recordCounts->push_back(count);
            count = 0;
            current_threshold += threshold_interval;
        }
    }
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::SeekToTime(
        DurationNs time) {
    //  Durations are signed. Make sure the given time is positive.
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
    if (multiplier < 0) {
        return SensorSessionPlayback::MULTIPLIER_NEGATIVE;
    }
    carVhalReplayMultiMsg_.trySend(multiplier);
    carVhalReplayMsg_.trySend(REPLAY_START);
    playThread_.reset(
            new android::base::FunctorThread([this]() { playSensor(); }));

    resetSession();

    playThread_->start();
    return SensorSessionPlayback::OK;
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
                            1000);
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
    return System::get()->getUnixTimeUs() * 1000;
}

}  // namespace sensorsessionplayback
}  // namespace android
