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
static constexpr int64_t MAX_SLEEP_DURATION = 1000;

SensorSessionPlayback::SensorSessionPlayback() {}

SensorSessionPlayback::~SensorSessionPlayback() {
    stopReplay();
}

// Load the binary proto
bool SensorSessionPlayback::loadFrom(std::string filename) {
    // Load into separate memory so that the old session remains valid if
    // loading fails.
    emulator::SensorSession newSession;
    uint64_t size = 0;

    ProtobufLoadResult loadResult = loadProtobuf(filename, newSession, &size);

    if (loadResult != ProtobufLoadResult::Success)
    {
        return false;
    }

    // Sort the sensor records by time.
    sensorRecordsSort(newSession.mutable_sensor_records());

    AutoLock lock(mCarVhalReplayLock);
    mSession = newSession;

    mCurrentElapsedTime = 0;
    mCurrentRecordIterator = mSession.sensor_records().begin();
    mCurrentSensorAggregate.Clear();

    return true;
}

std::vector<int> SensorSessionPlayback::getSensorRecordTimeLine(
        DurationNs interval) {
    std::vector<int> recordCounts;
    DurationNs currentThreshold = interval;
    DurationNs thresholdInterval = interval;

    AutoLock lock(mCarVhalReplayLock);
    int count = 0;
    for (auto it = mSession.sensor_records().begin();
         it != mSession.sensor_records().end(); it++) {
        count++;
        if (it->timestamp_ns() > currentThreshold) {
            recordCounts.push_back(count);
            count = 0;
            currentThreshold += thresholdInterval;
        }
    }
    lock.unlock();
    return recordCounts;
}

bool SensorSessionPlayback::seekToTime(DurationNs time) {
    //  Durations are signed. Make sure the given time is positive.
    if (time < 0) {
        return false;
    }

    emulator::SensorSession::SensorRecord timestampedRecord;
    timestampedRecord.set_timestamp_ns(time);

    AutoLock lock(mCarVhalReplayLock);

    google::protobuf::RepeatedPtrField<
            const emulator::SensorSession::SensorRecord>::iterator
            newRecordIterator = std::lower_bound(
                    mSession.sensor_records().begin(),
                    mSession.sensor_records().end(), timestampedRecord,
                    [](emulator::SensorSession::SensorRecord a,
                       emulator::SensorSession::SensorRecord b) {
                        return a.timestamp_ns() < b.timestamp_ns();
                    });
    mCurrentElapsedTime = time;

    // If we're seeking to the same place, we're done.
    if (newRecordIterator == mCurrentRecordIterator)
        return true;

    // If we're seeking forward, fast forward from our current position.
    if (newRecordIterator == mSession.sensor_records().end() ||
        newRecordIterator->timestamp_ns() >
                mCurrentRecordIterator->timestamp_ns()) {
        for (auto iter = mCurrentRecordIterator; iter != newRecordIterator;
             ++iter) {
            mCurrentSensorAggregate.MergeFrom(*iter);
        }
    } else {
        // Otherwise, reconstruct the current sensor record from the beginning.
        mCurrentRecordIterator = newRecordIterator;
        mCurrentSensorAggregate.Clear();
        for (auto iter = mSession.sensor_records().begin();
             iter != mCurrentRecordIterator; ++iter) {
            mCurrentSensorAggregate.MergeFrom(*iter);
        }
    }
    lock.unlock();
    return true;
}

bool SensorSessionPlayback::startReplay(float multiplier) {
    if (multiplier < 0) {
        return false;
    }
    mCarVhalReplayMultiMsg.trySend(multiplier);
    mCarVhalReplayMsg.trySend(REPLAY_START);
    mPlayThread.reset(
            new android::base::FunctorThread([this]() { playSensor(); }));

    mPlayThread->start();
    return true;
}

void SensorSessionPlayback::playSensor() {
    float multiplier;
    int replayState;
    mCarVhalReplayMultiMsg.tryReceive(&multiplier);
    mCarVhalReplayMsg.tryReceive(&replayState);
    AutoLock lock(mCarVhalReplayLock);

    DurationNs currentStartTime = getUnixTimeNs() - mCurrentElapsedTime;
    DurationNs wakeUpTime = 0;

    // Need add another channel msg for stop and start
    while (mCurrentRecordIterator != mSession.sensor_records().end() &&
           replayState == REPLAY_START) {
        mCurrentElapsedTime = (getUnixTimeNs() - currentStartTime) * multiplier;

        // Keep playing data until we've caught up.
        while ((mCurrentRecordIterator != mSession.sensor_records().end()) &&
               mCurrentElapsedTime > mCurrentRecordIterator->timestamp_ns()) {
            // Update the sensor record.
            mCurrentSensorAggregate.MergeFrom(*mCurrentRecordIterator);
            // Issue the callback if it's registered.
            if (mCallback) {
                // Unlock before sending out the last record to avoid conflict
                // with reset process
                if (mCurrentRecordIterator->timestamp_ns() ==
                    sessionDuration()) {
                    lock.unlock();
                }
                mCallback(*mCurrentRecordIterator);
            }

            // Advance to the next record
            wakeUpTime = currentStartTime +
                         mCurrentRecordIterator->timestamp_ns() -
                         getUnixTimeNs() + MAX_SLEEP_DURATION;
            mCurrentRecordIterator++;
        }
        // Sleep until the next record is ready. Max sleep duration is
        // MAX_SLEEP_DURATION
        if (mCurrentRecordIterator != mSession.sensor_records().end()) {
            DurationNs nextRecordAbsoluteTime =
                    currentStartTime + mCurrentRecordIterator->timestamp_ns() -
                    getUnixTimeNs();
            wakeUpTime = (wakeUpTime > nextRecordAbsoluteTime)
                                 ? nextRecordAbsoluteTime
                                 : wakeUpTime;

            mCarVhalReplayCV.timedWait(&mCarVhalReplayLock,
                                       wakeUpTime / US_TO_NS);
        }

        mCarVhalReplayMsg.tryReceive(&replayState);
    }
}

void SensorSessionPlayback::stopReplay() {
    mCarVhalReplayMsg.trySend(REPLAY_STOP);
    if (mPlayThread) {
        mPlayThread->wait();
    }
}

SensorSessionPlayback::DurationNs SensorSessionPlayback::getUnixTimeNs() {
    return System::get()->getUnixTimeUs() * US_TO_NS;
}

}  // namespace sensorsessionplayback
}  // namespace android
