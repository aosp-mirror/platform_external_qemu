// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/mp4/SensorLocationEventProvider.h"

#include <algorithm>                             // for max
#include <memory>                                // for unique_ptr, shared_ptr
#include <queue>                                 // for priority_queue
#include <utility>                               // for move
#include <vector>                                // for vector

#include "android/automation/AutomationController.h"
#include "android/automation/EventSource.h"      // for DurationNs
#include "android/base/Log.h"                    // for LogMessage, LOG, CHECK
#include "android/base/synchronization/Lock.h"   // for Lock
#include "android/hw-sensors.h"                  // for ANDROID_SENSOR_ACCEL...
#include "android/mp4/FieldDecodeInfo.h"         // for FieldDecodeInfo
#include "offworld.pb.h"  // for SensorDataPacketInfo

extern "C" {
#include "libavcodec/avcodec.h"                  // for AVPacket
}

namespace android {
namespace mp4 {

typedef base::AutoLock AutoLock;
typedef base::Lock Lock;
typedef emulator_automation::RecordedEvent RecordedEvent;
typedef emulator_automation::SensorOverrideEvent SensorOverrideEvent;
typedef ::offworld::DatasetInfo DatasetInfo;
typedef ::offworld::FieldInfo::Type Type;
typedef automation::DurationNs DurationNs;
typedef automation::NextCommandStatus NextCommandStatus;

struct SensorDecodeInfo {
    int streamIndex = -1;
    FieldDecodeInfo timestamp;
    FieldDecodeInfo value[3];
};

auto recorded_event_comparator = [](const RecordedEvent& a,
                                    const RecordedEvent& b) {
    return a.delay() > b.delay();
};

typedef std::priority_queue<RecordedEvent,
                            std::vector<RecordedEvent>,
                            decltype(recorded_event_comparator)>
        RecordedEventQueue;

// This class consumes AVPackets containing sensor and location event info and
// turn them into proto format that's consumable by AutomationController.
// For now we only support accelerometer, gyroscope and magnetometer events.
// TODO(haipengwu): provide location event support once it has been
// implemented in ARCore dataset
class SensorLocationEventProviderImpl : public SensorLocationEventProvider {
public:
    SensorLocationEventProviderImpl() {
        mEventQueue.reset(new RecordedEventQueue(recorded_event_comparator));
    };
    virtual ~SensorLocationEventProviderImpl(){};
    int init(const DatasetInfo& datasetInfo);
    void start() override { mStarted = true; }
    void stop() override { mStarted = false; }
    void startFromTimestamp(uint64_t startingTimestamp) override;
    void setEndingTimestamp(uint64_t endingTimestamp) override;
    int createEvent(const AVPacket* packet) override;
    RecordedEvent consumeNextCommand() override;
    NextCommandStatus getNextCommandDelay(DurationNs* outDelay) override;

private:
    int createSensorEvent(AndroidSensor sensor, uint8_t* data);

private:
    // A priority queue is used to keep events in ascending order
    // of timestamps when requested by AutomationController
    // because their creation are not necessarily in that order.
    std::unique_ptr<RecordedEventQueue> mEventQueue;
    Lock mLock;
    uint64_t mMinTimestamp = 0;
    uint64_t mMaxTimestamp = std::numeric_limits<uint64_t>::max();
    SensorDecodeInfo mSensorDecodeInfo[MAX_SENSORS];
    bool mStarted = false;
};

std::shared_ptr<SensorLocationEventProvider>
SensorLocationEventProvider::create(const DatasetInfo& datasetInfo) {
    auto provider = new SensorLocationEventProviderImpl();
    if (provider->init(datasetInfo) < 0) {
        LOG(ERROR) << "Failed to initialize event provider!";
        provider = nullptr;
    }
    std::shared_ptr<SensorLocationEventProvider> ret;
    ret.reset(provider);
    return std::move(ret);
}

int SensorLocationEventProviderImpl::init(const DatasetInfo& datasetInfo) {
    // Populate accelerometer stream decode info
    if (datasetInfo.has_accelerometer()) {
        mSensorDecodeInfo[ANDROID_SENSOR_ACCELERATION].streamIndex =
                datasetInfo.accelerometer().stream_index();

        auto SensorDecodeInfo = datasetInfo.accelerometer().sensor_packet();
        mSensorDecodeInfo[ANDROID_SENSOR_ACCELERATION].timestamp.offset =
                SensorDecodeInfo.timestamp().offset();
        mSensorDecodeInfo[ANDROID_SENSOR_ACCELERATION].timestamp.type =
                SensorDecodeInfo.timestamp().type();

        int valueSize = SensorDecodeInfo.value_size();
        for (int i = 0; i < valueSize; i++) {
            mSensorDecodeInfo[ANDROID_SENSOR_ACCELERATION].value[i].offset =
                    SensorDecodeInfo.value(i).offset();
            mSensorDecodeInfo[ANDROID_SENSOR_ACCELERATION].value[i].type =
                    SensorDecodeInfo.value(i).type();
        }
    }

    // Populate gyroscope stream decode info
    if (datasetInfo.has_gyroscope()) {
        mSensorDecodeInfo[ANDROID_SENSOR_GYROSCOPE].streamIndex =
                datasetInfo.gyroscope().stream_index();

        auto SensorDecodeInfo = datasetInfo.gyroscope().sensor_packet();
        mSensorDecodeInfo[ANDROID_SENSOR_GYROSCOPE].timestamp.offset =
                SensorDecodeInfo.timestamp().offset();
        mSensorDecodeInfo[ANDROID_SENSOR_GYROSCOPE].timestamp.type =
                SensorDecodeInfo.timestamp().type();

        int valueSize = SensorDecodeInfo.value_size();
        for (int i = 0; i < valueSize; i++) {
            mSensorDecodeInfo[ANDROID_SENSOR_GYROSCOPE].value[i].offset =
                    SensorDecodeInfo.value(i).offset();
            mSensorDecodeInfo[ANDROID_SENSOR_GYROSCOPE].value[i].type =
                    SensorDecodeInfo.value(i).type();
        }
    }

    // Populate magnetometer stream decode info
    if (datasetInfo.has_magnetic_field()) {
        mSensorDecodeInfo[ANDROID_SENSOR_MAGNETIC_FIELD].streamIndex =
                datasetInfo.magnetic_field().stream_index();

        auto SensorDecodeInfo = datasetInfo.magnetic_field().sensor_packet();
        mSensorDecodeInfo[ANDROID_SENSOR_MAGNETIC_FIELD].timestamp.offset =
                SensorDecodeInfo.timestamp().offset();
        mSensorDecodeInfo[ANDROID_SENSOR_MAGNETIC_FIELD].timestamp.type =
                SensorDecodeInfo.timestamp().type();

        int valueSize = SensorDecodeInfo.value_size();
        for (int i = 0; i < valueSize; i++) {
            mSensorDecodeInfo[ANDROID_SENSOR_MAGNETIC_FIELD].value[i].offset =
                    SensorDecodeInfo.value(i).offset();
            mSensorDecodeInfo[ANDROID_SENSOR_MAGNETIC_FIELD].value[i].type =
                    SensorDecodeInfo.value(i).type();
        }
    }

    return 0;
}

void SensorLocationEventProviderImpl::startFromTimestamp(
        uint64_t startingTimestamp) {
    AutoLock lock(mLock);
    mMinTimestamp = startingTimestamp;
    mStarted = true;
}

void SensorLocationEventProviderImpl::setEndingTimestamp(
        uint64_t endingTimestamp) {
    AutoLock lock(mLock);
    mMaxTimestamp = endingTimestamp;
}

int SensorLocationEventProviderImpl::createEvent(const AVPacket* packet) {
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (packet->stream_index == mSensorDecodeInfo[i].streamIndex) {
            return createSensorEvent(static_cast<AndroidSensor>(i),
                                     packet->data);
        }
    }
    LOG(ERROR) << ": Failed to create event!";
    return -1;
}

int SensorLocationEventProviderImpl::createSensorEvent(AndroidSensor sensor,
                                                       uint8_t* data) {
    RecordedEvent recordedEvent;
    auto sensorEvent = recordedEvent.mutable_sensor_override();

    // Set event type
    switch (sensor) {
        case ANDROID_SENSOR_ACCELERATION:
            sensorEvent->set_sensor(
                    SensorOverrideEvent::Sensor::
                            SensorOverrideEvent_Sensor_ACCELERATION);
            break;
        case ANDROID_SENSOR_GYROSCOPE:
            sensorEvent->set_sensor(
                    SensorOverrideEvent::Sensor::
                            SensorOverrideEvent_Sensor_GYROSCOPE);
            break;
        case ANDROID_SENSOR_MAGNETIC_FIELD:
            sensorEvent->set_sensor(
                    SensorOverrideEvent::Sensor::
                            SensorOverrideEvent_Sensor_MAGNETIC_FIELD);
            break;
        default:
            LOG(ERROR) << ": Unrecognized sensor type!";
            return -1;
    }

    // Set delay
    int timestampOffset = mSensorDecodeInfo[sensor].timestamp.offset;
    Type timestampType = mSensorDecodeInfo[sensor].timestamp.type;
    switch (timestampType) {
        case Type::FieldInfo_Type_INT_32: {
            auto timestampPtr =
                    reinterpret_cast<int32_t*>(data + timestampOffset);
            recordedEvent.set_delay(static_cast<uint64_t>(*timestampPtr));
            break;
        }
        case Type::FieldInfo_Type_INT_64: {
            auto timestampPtr =
                    reinterpret_cast<int64_t*>(data + timestampOffset);
            recordedEvent.set_delay(static_cast<uint64_t>(*timestampPtr));
            break;
        }
        case Type::FieldInfo_Type_FLOAT: {
            auto timestampPtr =
                    reinterpret_cast<float*>(data + timestampOffset);
            recordedEvent.set_delay(static_cast<uint64_t>(*timestampPtr));
            break;
        }
        case Type::FieldInfo_Type_DOUBLE: {
            auto timestampPtr =
                    reinterpret_cast<double*>(data + timestampOffset);
            recordedEvent.set_delay(static_cast<uint64_t>(*timestampPtr));
            break;
        }
        default:
            LOG(ERROR) << ": Unrecognized timestamp data type!";
            return -1;
    }

    // Set values
    auto value = sensorEvent->mutable_value();
    for (int i = 0; i < 3; i++) {
        int valueOffset = mSensorDecodeInfo[sensor].value[i].offset;
        if (valueOffset >= 0) {
            Type sensorValueType = mSensorDecodeInfo[sensor].value[i].type;
            switch (sensorValueType) {
                case Type::FieldInfo_Type_INT_32: {
                    auto valuePtr =
                            reinterpret_cast<int32_t*>(data + valueOffset);
                    value->add_data(static_cast<float>(*valuePtr));
                    break;
                }
                case Type::FieldInfo_Type_INT_64: {
                    auto valuePtr =
                            reinterpret_cast<int64_t*>(data + valueOffset);
                    value->add_data(static_cast<float>(*valuePtr));
                    break;
                }
                case Type::FieldInfo_Type_FLOAT: {
                    auto valuePtr =
                            reinterpret_cast<float*>(data + valueOffset);
                    value->add_data(*valuePtr);
                    break;
                }
                case Type::FieldInfo_Type_DOUBLE: {
                    auto valuePtr =
                            reinterpret_cast<double*>(data + valueOffset);
                    value->add_data(static_cast<float>(*valuePtr));
                    break;
                }
                default:
                    LOG(ERROR) << ": Unrecognized value data type!";
                    return -1;
            }
        } else if (i == 0) {
            LOG(ERROR) << ": Unrecognized decode info!";
            return -1;
        }
    }

    AutoLock lock(mLock);
    mEventQueue->push(recordedEvent);

    return 0;
}

RecordedEvent SensorLocationEventProviderImpl::consumeNextCommand() {
    AutoLock lock(mLock);
    CHECK(mEventQueue->size() != 0);
    RecordedEvent event = std::move(mEventQueue->top());
    mEventQueue->pop();
    return event;
}

NextCommandStatus SensorLocationEventProviderImpl::getNextCommandDelay(
        DurationNs* outDelay) {
    AutoLock lock(mLock);
    if (!mStarted || mEventQueue->size() == 0) {
        return NextCommandStatus::NONE;
    }

    auto currentTimestamp = mEventQueue->top().delay();
    *outDelay = (currentTimestamp < mMinTimestamp)
                        ? 0
                        : currentTimestamp - mMinTimestamp;
    mMinTimestamp = std::max(currentTimestamp, mMinTimestamp);
    if (currentTimestamp <= mMaxTimestamp) {
        return NextCommandStatus::OK;
    } else {
        return NextCommandStatus::PAUSED;
    }
}

}  // namespace mp4
}  // namespace android
