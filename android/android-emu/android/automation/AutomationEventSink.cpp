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

void AutomationEventSink::registerStream(Stream* stream,
                                         StreamEncoding encoding) {
    AutoLock lock(mLock);
    if (encoding == StreamEncoding::BinaryPbChunks) {
        mBinaryStreams.insert(stream);
    } else {
        mTextStreams.insert(stream);
    }
}

void AutomationEventSink::unregisterStream(Stream* stream) {
    AutoLock lock(mLock);
    if (!mTextStreams.erase(stream) && !mBinaryStreams.erase(stream)) {
        LOG(WARNING) << "Could not find stream.";
    }
}

void AutomationEventSink::recordPhysicalModelEvent(
        uint64_t timeNs,
        pb::PhysicalModelEvent& event) {
    pb::RecordedEvent recordedEvent;
    recordedEvent.set_stream_type(pb::RecordedEvent_StreamType_PHYSICAL_MODEL);
    recordedEvent.mutable_event_time()->set_timestamp(timeNs);
    *recordedEvent.mutable_physical_model() = event;

    handleEvent(recordedEvent);
}

void AutomationEventSink::handleEvent(pb::RecordedEvent& event) {
    AutoLock lock(mLock);
    if (!mBinaryStreams.empty()) {
        std::string binaryProto;
        if (!event.SerializeToString(&binaryProto)) {
            LOG(WARNING) << "Could not serialize event.";
            return;
        }

        for (auto& stream : mBinaryStreams) {
            stream->putString(binaryProto);
        }
    }

    if (!mTextStreams.empty() || VERBOSE_CHECK(automation)) {
        google::protobuf::TextFormat::Printer printer;
        printer.SetSingleLineMode(true);
        printer.SetUseShortRepeatedPrimitives(true);

        std::string textProto;
        if (!printer.PrintToString(event, &textProto)) {
            LOG(WARNING) << "Could not serialize event to string.";
            return;
        }

        VLOG(automation) << textProto;

        for (auto& stream : mTextStreams) {
            // Use write() instead of putString() because putString writes
            // length as binary before the data, and we want to maintain pure
            // text.
            stream->write(textProto.c_str(), textProto.size());
        }
    }
}

}  // namespace automation
}  // namespace android
