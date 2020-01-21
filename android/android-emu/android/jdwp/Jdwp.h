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
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace android {
namespace jdwp {

// https://docs.oracle.com/javase/7/docs/platform/jpda/jdwp/jdwp-protocol.html

// We only list those that are used

static const char* kJdwpHandshake = "JDWP-Handshake";
static const int kJdwpHandshakeSize = 14;  // exclude the ending 0
static const int kJdwpHeaderSize = 11;
static const int kJdwpReplyFlag = 0x80;

enum CommandSet {
    VirtualMachine = 0x1,
    EventRequest = 0xf,
    Event = 0x40,
    ExtensionBegin = 0x80,
};

enum VirtualMachineCommand {
    Version = 0x1,
    ClassBySignature = 0x2,
    AllClasses = 0x3,
    IDSizes = 0x7,
    Resume = 0x9,
    Capabilities = 0xc,
};

enum EventRequestCommand {
    Set = 0x1,
};

enum EventCommand {
    Composite = 0x64,
};

enum EventKind {
    Exception = 0x4,
    ThreadStart = 0x6,
    ThreadDeath = 0x7,
    ClassPrepare = 0x8,
    ClassUnload = 0x9,
};

enum SuspendPolicy {
    None = 0x0,
    EventThread = 0x1,
    All = 0x2,
};

// JDWP uses big-endian

template <typename _T>
_T readValFromBuffer(const uint8_t* buffer, int bytes) {
    // https://wiki.sei.cmu.edu/confluence/display/c/INT34-C.+Do+not+shift+an+expression+by+a+negative+number+of+bits+or+by+greater+than+or+equal+to+the+number+of+bits+that+exist+in+the+operand
    assert(bytes <= sizeof(_T) && "Too many bytes to fit in a _T.");
    _T val = 0;
    for (int i = 0; i < bytes; i++) {
        val |= static_cast<_T>(buffer[i]) << ((bytes - i - 1) << 3);
    }
    return val;
}

inline uint32_t uint32FromBuffer(const uint8_t* buffer) {
    return readValFromBuffer<uint32_t>(buffer, 4);
}

template <typename _T>
uint8_t* writeValToBuffer(uint8_t* buffer, _T val, int bytes) {
    for (size_t i = 0; i < bytes; i++) {
        buffer[0] = val >> ((bytes - i - 1) << 3) & 0xff;
        buffer++;
    }
    return buffer;
}

template <typename _T>
uint8_t* writeValToBuffer(uint8_t* buffer, _T val) {
    return writeValToBuffer(buffer, val, sizeof(val));
}

inline uint8_t* writeStrToBuffer(uint8_t* buffer, const char* str) {
    uint32_t len = strlen(str);
    buffer = writeValToBuffer(buffer, (uint32_t)len);
    memcpy(buffer, str, len);
    buffer += len;
    return buffer;
}

struct JdwpCommandHeader {
    uint32_t length;
    uint32_t id;
    uint8_t flags;
    uint8_t command_set;
    uint8_t command;
    void parseFrom(const uint8_t* buffer) {
        length = uint32FromBuffer(buffer);
        id = uint32FromBuffer(buffer + 4);
        flags = buffer[8];
        command_set = buffer[9];
        command = buffer[10];
    }
    void writeToBuffer(uint8_t* buffer) const {
        writeValToBuffer(buffer, length);
        writeValToBuffer(buffer + 4, id);
        buffer[8] = flags;
        buffer[9] = command_set;
        buffer[10] = command;
    }
};

struct JdwpIdSize {
    int32_t field_id_size;
    int32_t method_id_size;
    int32_t object_id_size;
    int32_t reference_typ_id_size;
    int32_t frame_id_size;
    void parseFrom(const uint8_t* buffer) {
        field_id_size = uint32FromBuffer(buffer);
        method_id_size = uint32FromBuffer(buffer + 4);
        object_id_size = uint32FromBuffer(buffer + 8);
        reference_typ_id_size = uint32FromBuffer(buffer + 12);
        frame_id_size = uint32FromBuffer(buffer + 16);
    }
};

struct JdwpEventRequestSet {
    uint8_t event_kind;
    uint8_t suspend_policy;
    int writeToBuffer(uint8_t* buffer,
                      uint64_t exception_id = 0,
                      const JdwpIdSize* id_size = nullptr) const {
        const uint8_t* buffer_head_ptr = buffer;
        *(buffer++) = event_kind;
        *(buffer++) = suspend_policy;
        uint8_t* request_num_pst = buffer;
        buffer += 4;
        uint32_t request_num = 0;
        uint32_t total_len = 6;
        if (exception_id) {
            request_num++;
            // Case ExceptionOnly - if modKind is 8:
            //      referenceTypeID	exceptionOrNull
            //      boolean	caught
            //      boolean	uncaught

            // uncaught doesn't really work in Android
            *(buffer++) = 8;
            buffer = writeValToBuffer(buffer, exception_id,
                                      id_size->reference_typ_id_size);
            *(buffer++) = 1;
            *(buffer++) = 1;
        }
        writeValToBuffer(request_num_pst, request_num);
        return buffer - buffer_head_ptr;
    }
};

struct JdwpAllClasses {
    struct JdwpClass {
        uint8_t ref_type_tag;
        uint64_t type_id;  // Actual size is referenceTypeID
        std::string signature;
        int32_t status;
    };
    std::vector<JdwpClass> classes;
    void parseFrom(const uint8_t* buffer, const JdwpIdSize& id_size) {
        uint32_t class_num = uint32FromBuffer(buffer);
        buffer += 4;
        classes.resize(class_num);
        for (auto& clazz : classes) {
            clazz.ref_type_tag = *(buffer++);
            clazz.type_id = readValFromBuffer<uint64_t>(
                    buffer, id_size.reference_typ_id_size);
            buffer += id_size.reference_typ_id_size;
            size_t string_len = readValFromBuffer<size_t>(buffer, 4);
            buffer += 4;
            clazz.signature = std::string((char*)buffer, string_len);
            buffer += string_len;
            clazz.status = readValFromBuffer<int32_t>(buffer, 4);
            buffer += 4;
        }
    }
};

}  // namespace jdwp
}  // namespace android
