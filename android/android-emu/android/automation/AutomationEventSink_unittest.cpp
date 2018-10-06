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
#include "android/base/testing/ProtobufMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestMemoryOutputStream.h"
#include "android/physics/PhysicalModel.h"

#include <gmock/gmock.h>
#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

using namespace android::automation;
using android::base::System;
using android::base::TestLooper;
using android::base::TestMemoryOutputStream;
using android::EqualsProto;

struct PhysicalModelDeleter {
    void operator()(PhysicalModel* physicalModel) {
        physicalModel_free(physicalModel);
    }
};

typedef std::unique_ptr<PhysicalModel, PhysicalModelDeleter> PhysicalModelPtr;

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

    pb::RecordedEvent receivedEvent;
    EXPECT_TRUE(google::protobuf::TextFormat::ParseFromString(
            str.str(), &receivedEvent));
    stream.reset();
    return receivedEvent;
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

    EXPECT_EQ(receivedEvent.delay(), kEventTime);
    EXPECT_THAT(receivedEvent.physical_model(), EqualsProto(sentEvent));

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

    EXPECT_EQ(receivedEvent.delay(), kEventTime);
    EXPECT_THAT(receivedEvent.physical_model(), EqualsProto(sentEvent));

    sink.unregisterStream(&textStream);
    sink.recordPhysicalModelEvent(kEventTime, sentEvent);
    EXPECT_TRUE(textStream.view().empty());
}

TEST(AutomationEventSink, TimeOffset) {
    TestLooper looper;
    PhysicalModelPtr model(physicalModel_new());
    std::unique_ptr<AutomationController> controller =
            AutomationController::createForTest(model.get(), &looper);

    constexpr uint64_t kEventTime1 = 1235;
    constexpr uint64_t kEventTime2 = 5000;

    AutomationEventSink& sink = controller->getEventSink();
    TestMemoryOutputStream binaryStream;
    sink.registerStream(&binaryStream, StreamEncoding::BinaryPbChunks);

    pb::PhysicalModelEvent event;
    event.set_type(pb::PhysicalModelEvent_ParameterType_AMBIENT_MOTION);
    event.mutable_target_value()->add_data(0.5f);

    sink.recordPhysicalModelEvent(kEventTime1, event);
    EXPECT_EQ(ReceiveBinaryEvent(binaryStream).delay(), kEventTime1);

    sink.recordPhysicalModelEvent(kEventTime2, event);
    EXPECT_EQ(ReceiveBinaryEvent(binaryStream).delay(),
              kEventTime2 - kEventTime1);

    sink.unregisterStream(&binaryStream);
}