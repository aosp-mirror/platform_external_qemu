// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/automation/AutomationController.h"

#include "android/automation/AutomationEventSink.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestMemoryOutputStream.h"
#include "android/physics/PhysicalModel.h"

#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

using namespace android::automation;
using android::base::System;
using android::base::TestLooper;
using android::base::TestMemoryOutputStream;

struct PhysicalModelDeleter {
    void operator()(PhysicalModel* physicalModel) {
        physicalModel_free(physicalModel);
    }
};

typedef std::unique_ptr<PhysicalModel, PhysicalModelDeleter> PhysicalModelPtr;

namespace emulator_automation {
bool operator==(const PhysicalModelEvent& msg_a,
                const PhysicalModelEvent& msg_b) {
    return (msg_a.GetTypeName() == msg_b.GetTypeName()) &&
           (msg_a.SerializeAsString() == msg_b.SerializeAsString());
}
}  // namespace emulator_automation

pb::RecordedEvent ReceiveBinaryEvent(TestMemoryOutputStream& stream) {
    auto str = stream.view();
    const size_t size = str.size();
    EXPECT_GE(size, 4);
    if (size > 4) {
        const uint32_t protoSize = uint32_t(str[0] << 24) |
                                   uint32_t(str[1] << 16) |
                                   uint32_t(str[2] << 8) | uint32_t(str[3]);
        EXPECT_EQ(protoSize, size - 4);

        pb::RecordedEvent receivedEvent;
        receivedEvent.ParseFromArray(str.data() + 4, protoSize);
        stream.reset();
        return receivedEvent;
    }

    return pb::RecordedEvent();
}

pb::RecordedEvent ReceiveTextEvent(TestMemoryOutputStream& stream) {
    auto str = stream.view();
    const size_t eol = str.find("\r\n");
    EXPECT_NE(eol, std::string::npos);

    if (eol != std::string::npos) {
        pb::RecordedEvent receivedEvent;
        EXPECT_TRUE(google::protobuf::TextFormat::ParseFromString(
                str.str(), &receivedEvent));
        stream.reset();
        return receivedEvent;
    }

    return pb::RecordedEvent();
}

void PhysicalModelEventEqual(uint64_t timeNs,
                             pb::RecordedEvent_StreamType type,
                             const pb::PhysicalModelEvent& expected,
                             const pb::RecordedEvent& event) {
    EXPECT_EQ(event.stream_type(), type);
    EXPECT_EQ(event.event_time().timestamp(), timeNs);
    EXPECT_EQ(event.physical_model(), expected);
}

// EventSink doesn't crash when there are no listeners.
TEST(AutomationEventSink, NoListeners) {
    TestLooper looper;
    PhysicalModelPtr model(physicalModel_new());
    std::unique_ptr<AutomationController> controller =
            AutomationController::createForTest(model.get(), &looper);

    pb::PhysicalModelEvent event;
    controller->getEventSink().recordPhysicalModelEvent(0, event);
}

TEST(AutomationEventSink, ReceiveBinaryEvent) {
    TestLooper looper;
    PhysicalModelPtr model(physicalModel_new());
    std::unique_ptr<AutomationController> controller =
            AutomationController::createForTest(model.get(), &looper);

    constexpr uint64_t kEventTime = 1235;

    AutomationEventSink& sink = controller->getEventSink();
    TestMemoryOutputStream binaryStream;
    sink.registerStream(&binaryStream, StreamEncoding::BinaryPbChunks);

    pb::PhysicalModelEvent sentEvent;
    sentEvent.set_type(pb::PhysicalModelEvent_ParameterType_AMBIENT_MOTION);
    sentEvent.mutable_target_value()->add_data(0.5f);
    sink.recordPhysicalModelEvent(kEventTime, sentEvent);

    pb::RecordedEvent receivedEvent = ReceiveBinaryEvent(binaryStream);

    PhysicalModelEventEqual(kEventTime,
                            pb::RecordedEvent_StreamType_PHYSICAL_MODEL,
                            sentEvent, receivedEvent);

    sink.unregisterStream(&binaryStream);
    sink.recordPhysicalModelEvent(kEventTime, sentEvent);
    EXPECT_TRUE(binaryStream.view().empty());
}

TEST(AutomationEventSink, ReceiveTextEvent) {
    TestLooper looper;
    PhysicalModelPtr model(physicalModel_new());
    std::unique_ptr<AutomationController> controller =
            AutomationController::createForTest(model.get(), &looper);

    constexpr uint64_t kEventTime = 1235;

    AutomationEventSink& sink = controller->getEventSink();
    TestMemoryOutputStream textStream;
    sink.registerStream(&textStream, StreamEncoding::TextPb);

    pb::PhysicalModelEvent sentEvent;
    sentEvent.set_type(pb::PhysicalModelEvent_ParameterType_AMBIENT_MOTION);
    sentEvent.mutable_target_value()->add_data(0.5f);
    sink.recordPhysicalModelEvent(kEventTime, sentEvent);

    pb::RecordedEvent receivedEvent = ReceiveTextEvent(textStream);

    PhysicalModelEventEqual(kEventTime,
                            pb::RecordedEvent_StreamType_PHYSICAL_MODEL,
                            sentEvent, receivedEvent);

    sink.unregisterStream(&textStream);
    sink.recordPhysicalModelEvent(kEventTime, sentEvent);
    EXPECT_TRUE(textStream.view().empty());
}
