// Copyright (C) 2018 The Android Open Source Project
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

#include "android/automation/AutomationEventSink.h"

#include "android/automation/AutomationController.h"
#include "android/base/Log.h"

#include <google/protobuf/text_format.h>

using namespace android::base;

namespace android {
namespace automation {

AutomationEventSink::AutomationEventSink(Looper* looper) : mLooper(looper) {}

void AutomationEventSink::shutdown() {
    AutoLock lock(mLock);
    mShutdown = true;
}

void AutomationEventSink::registerStream(Stream* stream,
                                         StreamEncoding encoding) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return;
    }

    if (encoding == StreamEncoding::BinaryPbChunks) {
        mBinaryStreams.insert(stream);
    } else {
        mTextStreams.insert(stream);
    }

    mLastEventTime[stream] = mLooper->nowNs(Looper::ClockType::kVirtual);
}

void AutomationEventSink::unregisterStream(Stream* stream) {
    AutoLock lock(mLock);
    if (!mTextStreams.erase(stream) && !mBinaryStreams.erase(stream)) {
        LOG(WARNING) << "Could not find stream.";
    }

    mLastEventTime.erase(stream);
}

void AutomationEventSink::recordPhysicalModelEvent(
        uint64_t timeNs,
        pb::PhysicalModelEvent& event) {
    pb::RecordedEvent recordedEvent;
    *recordedEvent.mutable_physical_model() = event;
    handleEvent(timeNs, recordedEvent);
}

void AutomationEventSink::handleEvent(uint64_t timeNs,
                                      const pb::RecordedEvent& event) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return;
    }

    for (auto& stream : mBinaryStreams) {
        pb::RecordedEvent modifiedEvent = event;
        uint64_t& lastEventTime = mLastEventTime[stream];
        modifiedEvent.set_delay(timeNs - mLastEventTime[stream]);
        lastEventTime = timeNs;

        std::string binaryProto;
        if (!modifiedEvent.SerializeToString(&binaryProto)) {
            LOG(WARNING) << "Could not serialize event.";
            return;
        }
        stream->putString(binaryProto);
    }

    if (!mTextStreams.empty() || VERBOSE_CHECK(automation)) {
        google::protobuf::TextFormat::Printer printer;
        printer.SetSingleLineMode(true);
        printer.SetUseShortRepeatedPrimitives(true);

        if (VERBOSE_CHECK(automation)) {
            std::string textProto;
            if (!printer.PrintToString(event, &textProto)) {
                LOG(WARNING) << "Could not serialize event to string.";
                return;
            }

            VLOG(automation) << textProto;
        }

        for (auto& stream : mTextStreams) {
            pb::RecordedEvent modifiedEvent = event;
            uint64_t& lastEventTime = mLastEventTime[stream];
            modifiedEvent.set_delay(timeNs - mLastEventTime[stream]);
            lastEventTime = timeNs;

            std::string textProto;
            if (!printer.PrintToString(modifiedEvent, &textProto)) {
                LOG(WARNING) << "Could not serialize event to string.";
                return;
            }

            // Use write() instead of putString() because putString writes
            // length as binary before the data, and we want to maintain pure
            // text.
            stream->write(textProto.c_str(), textProto.size());
        }
    }
}

uint64_t AutomationEventSink::getLastEventTimeForStream(Stream* stream) {
    return mLastEventTime[stream];
}

}  // namespace automation
}  // namespace android
