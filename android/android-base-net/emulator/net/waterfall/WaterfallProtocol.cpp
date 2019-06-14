// Copyright (C) 2019 The Android Open Source Project
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
#include "emulator/net/waterfall/WaterfallProtocol.h"

#include <stdio.h>

#include "emulator/net/SocketTransport.h"

#if DEBUG >= 2
#define DD(fmt, ...)                                                     \
    do {                                                                 \
        printf("WaterfallProtocol: %s:%d| " fmt "\n", __func__, __LINE__, \
               ##__VA_ARGS__);                                           \
    } while (0)
#define DD_BUF(buf, len)                                \
    do {                                                \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)
#else
#define DD(fmt, ...) (void)0
#define DD_BUF(buf, len) (void)0
#endif

namespace emulator {
namespace net {
bool WaterfallProtocol::write(SocketTransport* to, const std::string message) {
    std::string msg;
#ifndef __LITTLE_ENDIAN__
#error "You will need to convert the size to little endian here."
#endif
    uint32_t size = message.size();
    char* bytes = (char*)&size;
    msg.reserve(size + 4);
    msg += bytes[0];
    msg += bytes[1];
    msg += bytes[2];
    msg += bytes[3];
    msg += message;
    return to->send(msg);
}

void WaterfallProtocol::received(SocketTransport* from,
                                 const std::string object) {
    mBuffer += object;
    int size = mBuffer.size();
    DD_BUF(object.c_str(), object.size());
    if (size >= 4) {
#ifndef __LITTLE_ENDIAN__
#error "You will need to convert the buffer from a little endian here."
#endif
        uint32_t needed = *(uint32_t*)mBuffer.data();
        if (needed == 0) {
            DD("Protocol closure requested..");
            from->close();
            return;
        }

        // Keep eating away while we can!
        DD("Waiting for %d, have: %lu\n", needed, mBuffer.size());
        while (mBuffer.size() >= needed + 4) {
            std::string msg = mBuffer.substr(4, needed);
            if (mReceiver)
                mReceiver->received(from, msg);
            else {
                DD("Dropped, no receiver");
            }
            mBuffer.erase(0, needed + 4);
            needed = *(uint32_t*)mBuffer.data();
        }
    }
}

void WaterfallProtocol::stateConnectionChange(SocketTransport* connection,
                                              State current) {
    if (mReceiver)
        mReceiver->stateConnectionChange(connection, current);
}
}  // namespace net
}  // namespace emulator
