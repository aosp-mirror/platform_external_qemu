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
#include <string>
#include <vector>

namespace android {
namespace jdwp {

inline uint32_t uint32FromBuffer(const uint8_t* buffer) {
    return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

template <typename _T>
inline uint8_t* writeValToBuffer(uint8_t* buffer, _T val) {
    const size_t bytes = sizeof(val);
    for (size_t i = 0; i < bytes; i++) {
        buffer[0] = val >> ((bytes - i - 1) << 3) & 0xff;
        buffer ++;
    }
    return buffer;
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
        writeValToBuffer(buffer, length);
        writeValToBuffer(buffer + 4, id);
        buffer[8] = flags;
        buffer[9] = command_set;
        buffer[10] = command;
    }
};

struct JdwpIdSize {
    int32_t field_id_size;
    int32_t	method_id_size;
    int32_t	object_id_size;
    int32_t	reference_typ_id_size;
    int32_t	frame_id_size;
    void parseFrom(const uint8_t* buffer) {
        field_id_size = uint32FromBuffer(buffer);
        method_id_size = uint32FromBuffer(buffer + 4);
        object_id_size = uint32FromBuffer(buffer + 8);
        reference_typ_id_size = uint32FromBuffer(buffer + 12);
        frame_id_size = uint32FromBuffer(buffer + 16);
    }
};

struct JdwpEventRequestSet {
    struct JdwpClassMatch {
        std::string clazz;
    };
    uint8_t event_kind;
    uint8_t suspend_policy;
    //std::vector<JdwpClassMatch> class_matches;
    int writeToBuffer(uint8_t* buffer, bool break_on_uncaught_exception = false,
        JdwpIdSize* id_size = nullptr) {
        const uint8_t* buffer_head_ptr = buffer;
        *(buffer++) = event_kind;
        *(buffer++) = suspend_policy;
        uint8_t* request_num_pst = buffer;
        buffer += 4;
        uint32_t request_num = 0;
        uint32_t total_len = 6;
        if (break_on_uncaught_exception) {
            request_num ++;
            // Case ExceptionOnly - if modKind is 8:
            //      referenceTypeID	exceptionOrNull
            //      boolean	caught
            //      boolean	uncaught
            *(buffer++) = 8;
            memset(buffer, 0, id_size->reference_typ_id_size);
            buffer += id_size->reference_typ_id_size;
            *(buffer++) = 1;
            *(buffer++) = 1;
        }
        //buffer = buffer + total_len;
        //for (const auto& it : class_matches) {
        //    total_len += it.clazz.length() + 1;
        //    memcpy(buffer, it.clazz.c_str(), it.clazz.length() + 1);
        //    buffer += it.clazz.length() + 1;
        //}
        writeValToBuffer(request_num_pst, request_num);
        return buffer - buffer_head_ptr;
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
