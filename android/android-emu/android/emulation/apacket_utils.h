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

#pragma once

#include <vector>

#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace emulation {
#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

#define ADB_AUTH_TOKEN 1
#define ADB_AUTH_SIGNATURE 2
#define ADB_AUTH_RSAPUBLICKEY 3

#define A_VERSION_MIN 0x01000000
#define A_VERSION_SKIP_CHECKSUM 0x01000001
#define A_VERSION 0x01000001

struct amessage {
    unsigned command = 0;     /* command identifier constant      */
    unsigned arg0 = 0;        /* first argument                   */
    unsigned arg1 = 0;        /* second argument                  */
    unsigned data_length = 0; /* length of payload (0 is allowed) */
    unsigned data_check = 0;  /* checksum of data payload         */
    unsigned magic = 0;       /* command ^ 0xffffffff             */
};

struct apacket {
    amessage mesg = {};
    std::vector<uint8_t> data;
};

inline size_t packetSize(const apacket& packet) {
    CHECK(packet.mesg.data_length == packet.data.size());
    return sizeof(packet.mesg) + packet.mesg.data_length;
}

inline bool sendPacket(int s, const apacket* packet) {
    using android::base::socketSendAll;
    CHECK(packet->mesg.data_length == packet->data.size());
    if (!socketSendAll(s, &packet->mesg, sizeof(packet->mesg))) {
        return false;
    }
    if (packet->mesg.data_length &&
        !socketSendAll(s, packet->data.data(), packet->mesg.data_length)) {
        return false;
    }
    return true;
}

inline bool recvPacket(int s, apacket* packet) {
    using android::base::socketRecvAll;
    if (!socketRecvAll(s, &packet->mesg, sizeof(packet->mesg))) {
        return false;
    }
    packet->data.resize(packet->mesg.data_length);
    if (packet->mesg.data_length &&
        !socketRecvAll(s, packet->data.data(), packet->mesg.data_length)) {
        return false;
    }
    return true;
}

inline bool recvOkay(int s) {
    apacket connect_ok;
    if (!recvPacket(s, &connect_ok)) {
        return false;
    }
    if (connect_ok.mesg.command != ADB_OKAY) {
        return false;
    }
    return true;
}

}  // namespace emulation
}  // namespace android