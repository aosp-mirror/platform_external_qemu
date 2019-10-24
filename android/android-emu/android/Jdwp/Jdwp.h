// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <stdint.h>
#include <vector>

namespace android {
namespace jdwp {

inline uint32_t uint32FromBuffer(const uint8_t* buffer) {
    return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

struct JdwpCommandHeader {
    uint32_t length;
    uint32_t id;
    uint8_t flags;
    uint8_t command_set;
    uint8_t command;
    JdwpCommandHeader(const uint8_t* buffer) {
        length = uint32FromBuffer(buffer);
        id = uint32FromBuffer(buffer + 4);
        flags = buffer[8];
        command_set = buffer[9];
        command = buffer[10];
    }
    JdwpCommandHeader() {}
    void writeToBuffer(uint8_t* buffer) {
        buffer[0] = length >> 24;
        buffer[1] = length >> 16 & 0xff;
        buffer[2] = length >> 8 & 0xff;
        buffer[3] = length & 0xff;
        buffer[4] = id >> 24;
        buffer[5] = id >> 16 & 0xff;
        buffer[6] = id >> 8 & 0xff;
        buffer[7] = id & 0xff;
        buffer[8] = flags;
        buffer[9] = command_set;
        buffer[10] = command;
    }
};

struct JdwpIdSize {
    int32_t fieldIDSize;
    int32_t	methodIDSize;
    int32_t	objectIDSize;
    int32_t	referenceTypeIDSize;
    int32_t	frameIDSize;
    void parseFrom(const uint8_t* buffer) {
        fieldIDSize = uint32FromBuffer(buffer);
        methodIDSize = uint32FromBuffer(buffer + 4);
        objectIDSize = uint32FromBuffer(buffer + 8);
        referenceTypeIDSize = uint32FromBuffer(buffer + 12);
        frameIDSize = uint32FromBuffer(buffer + 16);
    }
};

/*struct JdwpCompositeCommand {
    struct Event {
        uint8_t event_kind;
        int32_t request_id;

    };
    uint8_t suspend_policy;
    int32_t event_id;
    std::vector<>
}*/

}
}
