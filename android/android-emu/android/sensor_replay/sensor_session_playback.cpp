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

#include "aemu/base/files/ScopedFd.h"
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
    mCurrentRecordIteratorPos = 0;
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
    int destIdx = 0;
    for (auto it = mSession.sensor_records().begin();
         it != mSession.sensor_records().end(); ++it, ++destIdx) {
        if (it->timestamp_ns() >= time) {
            break;
        }
    }
    mCurrentElapsedTime = time;

    // If we're seeking to the same place, we're done.
    if (destIdx == mCurrentRecordIteratorPos)
        return true;

    auto dest = mSession.sensor_records()[destIdx];
    auto current = mSession.sensor_records()[mCurrentRecordIteratorPos];

    // If we're seeking forward, fast forward from our current position.
    if (destIdx > mSession.sensor_records().size() ||
        dest.timestamp_ns() > current.timestamp_ns()) {
        auto end = std::min(destIdx, mSession.sensor_records().size());
        for (int idx = mCurrentRecordIteratorPos;  idx <= end; idx++) {
            mCurrentSensorAggregate.MergeFrom(mSession.sensor_records()[idx]);
        }
    } else {
        // Otherwise, reconstruct the current sensor record from the beginning.
        mCurrentSensorAggregate.Clear();
        int idx = 0;
        for (auto iter = mSession.sensor_records().begin();
             idx < mCurrentRecordIteratorPos; ++iter, ++idx) {
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
    while (mCurrentRecordIteratorPos != mSession.sensor_records().size() &&
           replayState == REPLAY_START) {
        mCurrentElapsedTime = (getUnixTimeNs() - currentStartTime) * multiplier;

        // Keep playing data until we've caught up.
        while ((mCurrentRecordIteratorPos !=
                mSession.sensor_records().size()) &&
               mCurrentElapsedTime >
                       mSession.sensor_records()[mCurrentRecordIteratorPos]
                               .timestamp_ns()) {
            // Update the sensor record.
            mCurrentSensorAggregate.MergeFrom(mSession.sensor_records()[mCurrentRecordIteratorPos]);
            // Issue the callback if it's registered.
            if (mCallback) {
                // Unlock before sending out the last record to avoid conflict
                // with reset process
                if (mSession.sensor_records()[mCurrentRecordIteratorPos].timestamp_ns() ==
                    sessionDuration()) {
                    lock.unlock();
                }
                mCallback(mSession.sensor_records()[mCurrentRecordIteratorPos]);
            }

            // Advance to the next record
            wakeUpTime = currentStartTime +
                         mSession.sensor_records()[mCurrentRecordIteratorPos].timestamp_ns() -
                         getUnixTimeNs() + MAX_SLEEP_DURATION;
            mCurrentRecordIteratorPos++;
        }
        // Sleep until the next record is ready. Max sleep duration is
        // MAX_SLEEP_DURATION
        if (mCurrentRecordIteratorPos < mSession.sensor_records().size()) {
            DurationNs nextRecordAbsoluteTime =
                    currentStartTime + mSession.sensor_records()[mCurrentRecordIteratorPos].timestamp_ns() -
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
