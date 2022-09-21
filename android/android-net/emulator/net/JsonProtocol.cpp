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
#include "JsonProtocol.h"

#include <stdio.h>

#include "SocketTransport.h"

namespace emulator {
namespace net {
bool JsonProtocol::write(SocketTransport* to, const json object) {
    std::string msg = (object.dump() + '\0');
    return to->send(msg);
}

void JsonProtocol::received(SocketTransport* from, const std::string object) {
    for (unsigned int i = 0; i < object.size(); i++) {
        char c = object.at(i);
        if (c == '\0') {
            json jmessage = json::parse(mBuffer, nullptr, false);
            if (!jmessage.is_discarded()) {
                mReceiver->received(from, std::move(jmessage));
            } else {
                // Log, notify?
            }
            mBuffer.clear();
        } else {
            mBuffer += c;
        }
    }
}

void JsonProtocol::stateConnectionChange(SocketTransport* connection,
                                         State current) {
    mReceiver->stateConnectionChange(connection, current);
}
}  // namespace net
}  // namespace emulator
