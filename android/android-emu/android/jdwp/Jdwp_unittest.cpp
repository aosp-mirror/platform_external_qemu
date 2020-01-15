// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/jdwp/Jdwp.h"

#include <gtest/gtest.h>

using namespace android::jdwp;

TEST(Jdwp, ReadBuffer) {
    const uint8_t kByteBuffer[8] = {0, 0, 0, 0, 0, 0, 0, 0x1};
    EXPECT_EQ(0x1, readValFromBuffer<uint32_t>(kByteBuffer + 4, 4));
    EXPECT_EQ(0x1, readValFromBuffer<uint64_t>(kByteBuffer, 8));
    EXPECT_EQ(0x1, uint32FromBuffer(kByteBuffer + 4));

    const uint8_t kByteBuffer2[8] = {0, 0, 0, 0, 0, 0, 0, 0x2};
    EXPECT_EQ(0x2, readValFromBuffer<uint32_t>(kByteBuffer2 + 4, 4));
    EXPECT_EQ(0x2, readValFromBuffer<uint64_t>(kByteBuffer2, 8));
    EXPECT_EQ(0x2, uint32FromBuffer(kByteBuffer2 + 4));
}

TEST(Jdwp, WriteBuffer) {
    const uint8_t kExpectedByteBuffer[4] = {0, 0, 0, 0x7};
    uint8_t actualBuffer[4];
    uint8_t* bufferPtr = writeValToBuffer(actualBuffer, (uint32_t)0x7);
    EXPECT_EQ(bufferPtr, actualBuffer + 4);
    EXPECT_EQ(0, memcmp(actualBuffer, kExpectedByteBuffer,
            sizeof(kExpectedByteBuffer)));
}

TEST(Jdwp, WriteStrBuffer) {
    const uint8_t kExpectedByteBuffer[8] = {0, 0, 0, 0x4,
            'H', 'E', 'L', 'O'};
    uint8_t actualBuffer[8];
    uint8_t* bufferPtr = writeStrToBuffer(actualBuffer, "HELO");
    EXPECT_EQ(bufferPtr, actualBuffer + 8);
    EXPECT_EQ(0, memcmp(actualBuffer, kExpectedByteBuffer,
            sizeof(kExpectedByteBuffer)));
}

TEST(Jdwp, CommandHeader) {
    const JdwpCommandHeader kHeader = {
        11, // length
        0x1, // id
        0x0, // flags
        CommandSet::VirtualMachine, // command_set
        VirtualMachineCommand::Version, // command
    };
    JdwpCommandHeader header;
    uint8_t buffer[kHeader.length];
    kHeader.writeToBuffer(buffer);
    header.parseFrom(buffer);
    EXPECT_EQ(kHeader.length, header.length);
    EXPECT_EQ(kHeader.id, header.id);
    EXPECT_EQ(kHeader.flags, header.flags);
    EXPECT_EQ(kHeader.command_set, header.command_set);
    EXPECT_EQ(kHeader.command, header.command);
}

TEST(Jdwp, IdSize) {
    const uint8_t kField[20] = {
        0, 0, 0, 8,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
    };
    const uint8_t kMethod[20] = {
        0, 0, 0, 4,
        0, 0, 0, 8,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
    };
    const uint8_t kObject[20] = {
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 8,
        0, 0, 0, 4,
        0, 0, 0, 4,
    };
    const uint8_t kReference[20] = {
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 8,
        0, 0, 0, 4,
    };
    const uint8_t kFrame[20] = {
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 4,
        0, 0, 0, 8,
    };
    JdwpIdSize idSize;
    idSize.parseFrom(kField);
    EXPECT_EQ(8, idSize.field_id_size);
    EXPECT_EQ(4, idSize.method_id_size);
    EXPECT_EQ(4, idSize.object_id_size);
    EXPECT_EQ(4, idSize.reference_typ_id_size);
    EXPECT_EQ(4, idSize.frame_id_size);
    idSize.parseFrom(kMethod);
    EXPECT_EQ(4, idSize.field_id_size);
    EXPECT_EQ(8, idSize.method_id_size);
    EXPECT_EQ(4, idSize.object_id_size);
    EXPECT_EQ(4, idSize.reference_typ_id_size);
    EXPECT_EQ(4, idSize.frame_id_size);
    idSize.parseFrom(kObject);
    EXPECT_EQ(4, idSize.field_id_size);
    EXPECT_EQ(4, idSize.method_id_size);
    EXPECT_EQ(8, idSize.object_id_size);
    EXPECT_EQ(4, idSize.reference_typ_id_size);
    EXPECT_EQ(4, idSize.frame_id_size);
    idSize.parseFrom(kReference);
    EXPECT_EQ(4, idSize.field_id_size);
    EXPECT_EQ(4, idSize.method_id_size);
    EXPECT_EQ(4, idSize.object_id_size);
    EXPECT_EQ(8, idSize.reference_typ_id_size);
    EXPECT_EQ(4, idSize.frame_id_size);
    idSize.parseFrom(kFrame);
    EXPECT_EQ(4, idSize.field_id_size);
    EXPECT_EQ(4, idSize.method_id_size);
    EXPECT_EQ(4, idSize.object_id_size);
    EXPECT_EQ(4, idSize.reference_typ_id_size);
    EXPECT_EQ(8, idSize.frame_id_size);
}

TEST(Jdwp, EventRequest) {
    const JdwpEventRequestSet kEventRequest = {
        EventKind::Exception,
        SuspendPolicy::All,
    };
    const uint32_t kExceptionId = 0x7;
    uint8_t buffer[100];
    const JdwpIdSize kIdSize4BytesException = {
        0x2, 0x2, 0x2, 0x4, 0x2,
    };
    const uint8_t kExpected4BytesId[] = {
        kEventRequest.event_kind, kEventRequest.suspend_policy,
        0, 0, 0, 0x1, // Num of events
        0x8, // Exception Only
        0, 0, 0, kExceptionId,
        0x1, 0x1,
    };
    int bytesWritten = kEventRequest.writeToBuffer(
        buffer, kExceptionId, &kIdSize4BytesException
    );
    EXPECT_EQ(sizeof(kExpected4BytesId), bytesWritten);
    EXPECT_EQ(0, memcmp(kExpected4BytesId, buffer,
            sizeof(kExpected4BytesId)));

    const JdwpIdSize kIdSize8BytesException = {
        0x2, 0x2, 0x2, 0x8, 0x2,
    };
    const uint8_t kExpected8BytesId[] = {
        kEventRequest.event_kind, kEventRequest.suspend_policy,
        0, 0, 0, 0x1, // Num of events
        0x8, // Exception Only
        0, 0, 0, 0, 0, 0, 0, kExceptionId,
        0x1, 0x1,
    };
    bytesWritten = kEventRequest.writeToBuffer(
        buffer, kExceptionId, &kIdSize8BytesException
    );
    EXPECT_EQ(sizeof(kExpected8BytesId), bytesWritten);
    EXPECT_EQ(0, memcmp(kExpected8BytesId, buffer,
            sizeof(kExpected8BytesId)));
}

TEST(Jdwp, AllClasses) {
    JdwpAllClasses allClasses;
    const JdwpIdSize kIdSize8BytesClass = {
        0x2, 0x2, 0x2, 0x8, 0x2,
    };
    const char* kName = "LTestClass;";
    const uint8_t kBuffer[] = {
        0, 0, 0, 0x1, // class number
        0x2, // refTypeTag
        0, 0, 0, 0, 0, 0, 0, 0x3,// id
        0, 0, 0, (uint8_t)strlen(kName),
        'L', 'T', 'e', 's', 't', 'C', 'l', 'a', 's', 's', ';',
        0, 0, 0, 0x4, // status
    };
    allClasses.parseFrom(kBuffer, kIdSize8BytesClass);
    EXPECT_EQ(1, allClasses.classes.size());
    EXPECT_EQ(0x2, allClasses.classes[0].ref_type_tag);
    EXPECT_EQ(0x3, allClasses.classes[0].type_id);
    EXPECT_STREQ(kName, allClasses.classes[0].signature.c_str());
    EXPECT_EQ(0x4, allClasses.classes[0].status);
}
